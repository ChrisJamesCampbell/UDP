#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <time.h>

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 100

#define CPU_LOAD__AVG_SMOOTHER 0.5
#define PACKETS_PM_SMOOTHER 0.5
#define PROPORTIONAL_DISK_ACTIVITY_AVG_SMOOTHER 0.5
#define PROPORTIONAL_BANDWIDTH_AVG_SMOOTHER 0.5
#define PROPORTIONAL_FREE_MEM_AVG_SMOOTHER 0.5

//variable used to store the type of machine that sent
//the incoming packet
static char *machine;

time_t unix_time_now()
{
   time_t now;

   /* Get current time */
   time(&now);

  return now;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



//the struct that will conatin metrics which will be used
//to calculate state of system
struct sys_info     
{
    //all fields identical to talker struct sysinfo
    char cpu_load; 
    double free_mem;
    double proportional_free_mem;
    int machine_type;
    long disk_activity;
    double proportional_disk_activity; 
    double instantaneous_bandwidth;
    double proportional_bandwidth;
    
    //fields unique to listener
    time_t packet_time_stamp;
    double packets_per_minute;
    
};


static void initialise_sys_info(struct sys_info *sys_info) 
{
    
    sys_info->cpu_load = 0;
    sys_info->free_mem = 0.0;
    sys_info->proportional_free_mem = 0.0;
    sys_info->machine_type = 0;
    sys_info->disk_activity = 0;
    sys_info->proportional_disk_activity = 0.0; //disk activity
    sys_info->instantaneous_bandwidth = 0.0;
    sys_info->proportional_bandwidth = 0.0;
    
    sys_info->packet_time_stamp = unix_time_now();
    sys_info->packets_per_minute = 0.0;
    return;
    
}


static void save_data(struct sys_info *old_data, struct sys_info *new_data)
{
   //calculates cpu_load avaerage with cpu load average smoother  constant
    old_data->cpu_load = old_data->cpu_load * CPU_LOAD__AVG_SMOOTHER + 
    (double)new_data->cpu_load * (1 - CPU_LOAD__AVG_SMOOTHER);
    
    //calculates packets per minute with the packets per minute smoother constant
    old_data->packets_per_minute = old_data->packets_per_minute * PACKETS_PM_SMOOTHER
    + (1 - PACKETS_PM_SMOOTHER) * (60 / (unix_time_now() - old_data->packet_time_stamp));
    
    //updates packet time stamp
    old_data->packet_time_stamp = unix_time_now();
    
    //calculates proportional disk activity average
    old_data->proportional_disk_activity = old_data->proportional_disk_activity * PROPORTIONAL_DISK_ACTIVITY_AVG_SMOOTHER +
    new_data->proportional_disk_activity * (1 - PROPORTIONAL_DISK_ACTIVITY_AVG_SMOOTHER);
    
    //calculates proportional bandwidth average
    old_data->proportional_bandwidth = old_data->proportional_bandwidth * PROPORTIONAL_BANDWIDTH_AVG_SMOOTHER + 
    new_data->proportional_bandwidth * (1 - PROPORTIONAL_BANDWIDTH_AVG_SMOOTHER);
    
    //calculates proportional free memory average
    old_data->proportional_free_mem = old_data->proportional_free_mem * PROPORTIONAL_FREE_MEM_AVG_SMOOTHER +
    new_data->proportional_free_mem * (1 - PROPORTIONAL_FREE_MEM_AVG_SMOOTHER);
}

//method for determining which machine the packet has been received from
static void determine_machine(struct sys_info *sys_info) 
{
    if(sys_info->machine_type == 1)
    {
        machine = "Batch Robot";
    }
      
    if(sys_info->machine_type == 2)
    {
        machine = "Web Server";
    }
      
    if(sys_info->machine_type == 3)
    {
        machine = "Database Server";
    }
      
    if(sys_info->machine_type == 4)
    {
        machine = "Application Server";
    }
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    
   //sets the received buffer as type sys_info and names it as a packet
   struct sys_info *new_packet = (struct sys_info *)buf;
   
   struct sys_info old_data; //struct that stores data about all machines 
   initialise_sys_info(&old_data);
   
   //structs that will store data for specific machines
   //br = batch robot, ws = web server, ds = database server
   //as = application server
   struct sys_info old_br_data; 
   initialise_sys_info(&old_br_data);
   
   struct sys_info old_ws_data; 
   initialise_sys_info(&old_ws_data);
   
   struct sys_info old_ds_data; 
   initialise_sys_info(&old_ds_data);
   
   struct sys_info old_as_data; 
   initialise_sys_info(&old_as_data);
   
   
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
        	p->ai_protocol)) == -1) 
        {
            perror("listener: socket");
            continue;
    	}

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) 
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    
    while(1)
    {
        
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) 
        {
            perror("recvfrom");
            sleep(1);
            continue;
        }
        
       /* if(numbytes != (sizeof(struct sys_info)))
        {
            sleep(1);
            continue;
        }*/
        
        //set time received
        new_packet->packet_time_stamp = unix_time_now();
        
        //saves information into old_data struct and simultaneously produces metrics
        save_data(&old_data, new_packet);
        
        //finds out which type of machine sent the packet
        determine_machine(new_packet);
        
        //saves the information to the relevant specific machine struct 
        switch(new_packet->machine_type)
        {
            case 1:
                save_data(&old_br_data, new_packet);
                break;
            case 2:
                save_data(&old_ws_data, new_packet);
                break;
            case 3:
                save_data(&old_ds_data, new_packet);
                break;
            case 4:
                save_data(&old_as_data, new_packet);
                break;
               
        }
        
        FILE *fp;
        
        fp = fopen("/root/UDP/stored_sys_info", "w");
        
        fprintf(fp, "{\n \t \"CPU Load\" : \" %d \" ,"
                  " \n \t \"Proportional Free Memory\" : \"%lf\" ,"
                  " \n \t \"Proportional Disk Activity\" : \"%d\" ,"
                  " \n \t \"Proportional Bandwidth\" : \"%lf\" ,"
                  " \n \t \"Roles\" : [ "
                  " \n \t \t {\"Role ID\" : \"1\" , "
                  " \n \t \t \"CPU Load\" : \"%d\" , "
                  " \n \t \t \"Proportional Free Memory\" : \"%lf\" ,"
                  " \n \t \t \"Proportional Disk Activity\" : \"%lf\" ,"
                  " \n \t \t \"Proportional Bandwidth\" : \"%lf\" }"
                  " \n \t \t ]"
                  "\n }", 
               (int)old_data.cpu_load, new_packet->proportional_free_mem,
               new_packet->proportional_disk_activity, new_packet->proportional_bandwidth,
               old_br_data.cpu_load);
        
        fclose(fp);
        
        

        printf("\nRecieved packet from IP address %s\n",
            inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s));
        printf("The Packet was %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        printf("The packet was sent by a: %s\n", machine);
        printf("It was recieved at: %f \n", (double)new_packet->packet_time_stamp);
        printf("Packets arriving per minute is:  %f \n", old_data.packets_per_minute);
        printf("\nThe packet conatins: \n");
        printf("Instantaneous CPU Load: %d %%", (int)new_packet->cpu_load);
        printf("\nThe Free Memory on this machine was: %lf", (double)new_packet->free_mem);
        printf("\nThe Proportional Free Memory on this machine was: %lf %%", new_packet->proportional_free_mem);
        
        printf("\nAverage CPU load is: %d%% ", (int)old_data.cpu_load);
        
        printf("\nInstantaneous Disk activity was:  %d (reads/writes)", new_packet->disk_activity);
        printf("\nProportional Disk activity was: %lf %%", new_packet->proportional_disk_activity);
        printf("\nInstantaneous bandwidth was:  %lf bps)", new_packet->instantaneous_bandwidth);
        printf("\nProportional bandwidth was: %lf %% \n \n", new_packet->proportional_bandwidth);

    }
    close(sockfd);

    return 0;
}
