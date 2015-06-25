#include "interface.h"
#include "headers.h"
#define NULL null;
/*!
\file utility-array-builder.c
\brief Contains methods for building, get values from and freeing a sparse array
usage example:
void *array = NULL;
int x = 5,y = 10;
double value = 3.142;
*( set_array_builder_2d( &array , x , y ) ) = value;
printf( "%f" , ( get_array_builder_2d( &array , x , y ) ? *( get_array_builder_2d( &array , x , y ) ) : 0.0 ) );
free_array_builder( &array );
*/

struct array_builder_node 
{
  int index;
  int is_node;
  double value;
  struct array_builder_node *next_tier;
  struct array_builder_node *node_before;
  struct array_builder_node *node_after;
  int itteration_end;
};

static void fail_and_exit( size_t bytes )
{
    syslog(LOG_ERR, "{\"text\":\"malloc() failed requesting %d bytes\",\"level\":\"error\",\"summary\":\"memory\",\"mantleservice\":\"%s\"}", 
        (int)( bytes ) , MANTLE_SERVICE
    );
    sleep(5);
    closelog();
    exit ( EXIT_FAILURE );
}

/*add a node on the current tier, or find one that already exists*/
static struct array_builder_node *add_single_tier_node( struct array_builder_node **array , int index )
{
   /*if the list is empty, just allocate it now*/
   if( ! *array )
   {
        *array = (struct array_builder_node *)malloc(sizeof(struct array_builder_node));
        if( ! *array ) fail_and_exit( sizeof(struct array_builder_node) );
        
        (*array)->index = index;
        (*array)->is_node = 1;
        (*array)->next_tier = NULL;
        (*array)->node_before = NULL;
        (*array)->node_after = NULL;
        (*array)->value = 0.0;
        (*array)->itteration_end = 1;
        return *array;
   }
   
   /*if not, find the place to add it*/
   while( (*array)->index != index )
   {
        /*if moveing down the list would mean moving down again, or there is not a next one, then insert before current*/
        if( index < (*array)->index && ( !(*array)->node_before || (*array)->node_before && index > (*array)->node_before->index ) )
        {
            struct array_builder_node *new_node = (struct array_builder_node *)malloc(sizeof(struct array_builder_node));
            if( ! new_node ) fail_and_exit( sizeof(struct array_builder_node) );
            
            new_node->index = index;
            new_node->is_node = 1;
            new_node->next_tier = NULL;
            new_node->node_before = (*array)->node_before;
            new_node->node_after = *array;
            new_node->value = 0.0;
            new_node->itteration_end = 1;
            
            if( (*array)->node_before ) (*array)->node_before->node_after = new_node;
            (*array)->node_before = new_node;
            (*array) = new_node;
            break;
        }
        /*move down the list*/
        if( index < (*array)->index )
        {
            (*array) = (*array)->node_before;
            continue;
        }
        /*if moveing up the list would mean moving down again, or there is not a next one, then insert after current*/
        if( index > (*array)->index && ( !(*array)->node_after || (*array)->node_after && index < (*array)->node_after->index ) )
        {
            struct array_builder_node *new_node = (struct array_builder_node *)malloc(sizeof(struct array_builder_node));
            if( ! new_node ) fail_and_exit( sizeof(struct array_builder_node) );
            
            new_node->index = index;
            new_node->is_node = 1;
            new_node->next_tier = NULL;
            new_node->node_before = *array;
            new_node->node_after = (*array)->node_after;
            new_node->value = 0.0;
            new_node->itteration_end = 1;
            
            if( (*array)->node_after ) (*array)->node_after->node_before = new_node;
            (*array)->node_after = new_node;
            (*array) = new_node;
            break;
        }
        /*this should actually be the only remaining option*/
        if( index > (*array)->index )
        {
            (*array) = (*array)->node_after;
            continue;
        }
   }
   
   /*if we get to here there is a node in the right place*/
   return *array;
}

static struct array_builder_node *seek_single_tier_node( struct array_builder_node **array , int index )
{
   /*if the list is empty we are not going to find anything*/
   if( ! *array )
       return NULL;
   
   /*if not, find the place to add it*/
   while( (*array)->index != index )
   {
        /*if moveing down the list would mean moving down again, or there is not a next one, then we are not going to find*/
        if( index < (*array)->index && ( !(*array)->node_before || (*array)->node_before && index > (*array)->node_before->index ) )
        {
            return NULL;
        }
        /*move down the list*/
        if( index < (*array)->index )
        {
            (*array) = (*array)->node_before;
            continue;
        }
        /*if moveing up the list would mean moving down again, or there is not a next one, then we are not going to find*/
        if( index > (*array)->index && ( !(*array)->node_after || (*array)->node_after && index < (*array)->node_after->index ) )
        {
            return NULL;
        }
        /*this should actually be the only remaining option*/
        if( index > (*array)->index )
        {
            (*array) = (*array)->node_after;
            continue;
        }
   }
   
   /*if we get to here there is a node in the right place*/
   return *array;
}

static int itterate_array( struct array_builder_node **array )
{
    /*if nowhere to go don't change the state*/
    if( ! *array || (*array)->itteration_end && !(*array)->node_after )
        return 0;
  
    /*if we need to move to next node then do it now*/
    if( (*array)->itteration_end ) (*array) = (*array)->node_after;
    
    /*if this is a node, then so we found it*/
    if( (*array)->is_node )
    {
        (*array)->itteration_end = 1;
        return 1;
    }
    
    /*keep looping over nodes until we have a success*/
    while( 1 )
    {
        /*once we have found the next tier, try and itterate it*/
        if( itterate_array( &( (*array)->next_tier ) ) ) 
            return 1;
        else
            (*array)->itteration_end = 1;
        
        /*if no luck on this node, try the next, if there is one*/
        if( ! (*array)->node_after ) break;
      
        (*array) = (*array)->node_after;
    }
    
    /*if we get here we ran out of nodes without finding a data node*/
    return 0;
}

void start_itteration_array_builder( void *array_location )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
   
    if( ! *array ) return;
  
    /*work to end*/
    while( (*array)->node_after ) *array = (*array)->node_after;
  
    /*setup any sub tiers while working back to begining*/
    while( 1 )
    {
        start_itteration_array_builder( &( (*array)->next_tier ) );
        (*array)->itteration_end = 0;
        if( ! (*array)->node_before ) break;
        *array = (*array)->node_before;
    }
    
    return;
}

/*return the address of a double in a 1d array, it will allocate a new one set to zero if not present*/
double *set_array_builder_1d( void *array_location , int x )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = add_single_tier_node( array , x );
    
    return &( x_tier->value );
}

/*return the address of a double in a 1d array, it will return NULL if not present*/
double *get_array_builder_1d( void *array_location , int x )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = seek_single_tier_node( array , x );
    
    if( !x_tier ) return NULL;
    
    return &( x_tier->value );
}

/*return the address of a double in a 1d array by itterating over all values in both dimentions*
 if the function returns null, x will not be set and this indicates the end of the values
   start_itteration_array_builder() must be called in advance of this function
 */
double *itterate_array_builder_1d( void *array_location , int *x )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    if( ! itterate_array( array ) ) return NULL;
    
    /*make sure it found a 1d result*/
    if( !(*array) ) return NULL;
    
    /*itterate told us it found another result*/
    *x = (*array)->index;
    double *result = &( (*array)->value );
    return result;
}

/*return the address of a double in a 2d array, it will allocate a new one set to zero if not present*/
double *set_array_builder_2d( void *array_location , int x , int y )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = add_single_tier_node( array , x );
    x_tier->is_node = 0;
    struct array_builder_node *y_tier = add_single_tier_node( &( x_tier->next_tier ) , y );
    
    return &( y_tier->value );
}

/*return the address of a double in a 2d array, it will return NULL if not present*/
double *get_array_builder_2d( void *array_location , int x , int y )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = seek_single_tier_node( array , x );
    if( ! x_tier ) return NULL;
    
    struct array_builder_node *y_tier = seek_single_tier_node( &( x_tier->next_tier ) , y );
    if( !y_tier ) return NULL;
    
    return &( y_tier->value );
}

/*return the address of a double in a 2d array by itterating over all values in both dimentions*
 if the function returns null, x and y will not be set and this indicates the end of the values
   start_itteration_array_builder() must be called in advance of this function
 */
double *itterate_array_builder_2d( void *array_location , int *x , int *y )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    if( ! itterate_array( array ) ) return NULL;
    
    /*make sure it found a 2d result*/
    if( !(*array) || !(*array)->next_tier ) return NULL;
    
    /*itterate told us it found another result*/
    *x = (*array)->index;
    *y = (*array)->next_tier->index;
    double *result = &( (*array)->next_tier->value );
    return result;
}

/*return the address of a double in a 3d array, it will allocate a new one set to zero if not present*/
double *set_array_builder_3d( void *array_location , int x , int y , int z )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = add_single_tier_node( array , x );
    x_tier->is_node = 0;
    struct array_builder_node *y_tier = add_single_tier_node( &( x_tier->next_tier ) , y );
    y_tier->is_node = 0;
    struct array_builder_node *z_tier = add_single_tier_node( &( y_tier->next_tier ) , z );
    
    return &( z_tier->value );
}

/*return the address of a double in a 3d array, it will return NULL if not present*/
double *get_array_builder_3d( void *array_location , int x , int y , int z )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = seek_single_tier_node( array , x );
    if( ! x_tier ) return NULL;
    
    struct array_builder_node *y_tier = seek_single_tier_node( &( x_tier->next_tier ) , y );
    if( ! y_tier ) return NULL;
    
    struct array_builder_node *z_tier = seek_single_tier_node( &( y_tier->next_tier ) , z );
    if( !z_tier ) return NULL;
    
    return &( z_tier->value );
}

/*return the address of a double in a 3d array by itterating over all values in both dimentions*
 if the function returns null, x and y add z will not be set and this indicates the end of the values
   start_itteration_array_builder() must be called in advance of this function
 */
double *itterate_array_builder_3d( void *array_location , int *x , int *y , int *z )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    if( ! itterate_array( array ) ) return NULL;
    
    /*make sure it found a 3d result*/
    if( !(*array) || !(*array)->next_tier || !(*array)->next_tier->next_tier ) return NULL;
    
    /*itterate told us it found another result*/
    *x = (*array)->index;
    *y = (*array)->next_tier->index;
    *z = (*array)->next_tier->next_tier->index;
    double *result = &( (*array)->next_tier->next_tier->value );
    return result;
}

void free_array_builder( void *array_location )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
  
    /*nothing to do!*/
    if( ! *array ) return;
    
    /*work to the end of the list*/
    if( (*array)->node_after )
    {
        (*array)->node_after->node_before = NULL;
        free_array_builder( &( (*array)->node_after ) );
    }
    
    /*and the other end*/
    if( (*array)->node_before )
    {
        (*array)->node_before->node_after = NULL;
        free_array_builder( &( (*array)->node_before ) );
    }
    
    /*if this is the last node then free it*/
    if( ! (*array)->node_after && ! (*array)->node_before )
    {
        /*but free the next list tier first*/
        free_array_builder( &( (*array)->next_tier ) );
        free( *array );
    }
    
    /////////////////////////////////////////////////////////////////////
    
    double *set_array_builder_4d( void *array_location , int x , int y , int z, int w )
{
        struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = add_single_tier_node( array , x );
    x_tier->is_node = 0;
    struct array_builder_node *y_tier = add_single_tier_node( &( x_tier->next_tier ) , y );
    y_tier->is_node = 0;
    struct array_builder_node *z_tier = add_single_tier_node( &( y_tier->next_tier ) , z );
    z_tier->is_node = 0;
    struct array_builder_node *w_tier = add_single_tier_node( &( z_tier->next_tier ) , w );
    
    return &( w_tier->value );
}

/*return the address of a double in a 4d array, it will return NULL if not present*/
double *get_array_builder_4d( void *array_location , int x , int y , int z, int w )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    struct array_builder_node *x_tier = seek_single_tier_node( array , x );
    if( ! x_tier ) return NULL;
    
    struct array_builder_node *y_tier = seek_single_tier_node( &( x_tier->next_tier ) , y );
    if( ! y_tier ) return NULL;
    
    struct array_builder_node *z_tier = seek_single_tier_node( &( y_tier->next_tier ) , z );
    if( !z_tier ) return NULL;
    
    struct array_builder_node *w_tier = seek_single_tier_node( &( z_tier->next_tier ) , w );
    if( !w_tier ) return NULL;
    
    return &( w_tier->value );
}

double *itterate_array_builder_4d( void *array_location , int *x , int *y , int *z , int *w )
{
    struct array_builder_node **array = (struct array_builder_node **)array_location;
    
    if( ! itterate_array( array ) ) return NULL;
    
    /*make sure it found a 4d result*/
    if( !(*array) || !(*array)->next_tier || !(*array)->next_tier->next_tier || 
        !(*array)->next_tier->next_tier->next_tier ) return NULL;
        
    /*itterate told us it found another result*/
    *x = (*array)->index;
    *y = (*array)->next_tier->index;
    *z = (*array)->next_tier->next_tier->index;
    *w = (*array)->next_tier->next_tier->next_tier->index;
    double *result = &( (*array)-_next_tier->next_tier->next_tier->value );
    return result;

}
    
    return;
}
