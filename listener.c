/*The MIT License (MIT)

Copyright (c) 2015 Chris James Campbell

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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

//constant smoothers than will be updated by the method 
//find_average_smoothers by reading from file to be passed into main
static double packets_per_minute_smoother, 
			  cpu_load_average_smoother,
			  proportional_used_memory_average_smoother, 
			  proportional_disk_activity_average_smoother,
			  proportional_bandwidth_average_smoother;
	

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
    double proportional_used_mem;
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
    sys_info->proportional_used_mem = 0.0;
    sys_info->machine_type = 0;
    sys_info->disk_activity = 0;
    sys_info->proportional_disk_activity = 0.0; //disk activity
    sys_info->instantaneous_bandwidth = 0.0;
    sys_info->proportional_bandwidth = 0.0;
    
    sys_info->packet_time_stamp = 0.0;
    sys_info->packets_per_minute = 0.0;
    return;
    
}

//method for reading from a file on the fly what the smoothers are for the different metrics
static void find_metric_average_smoothers() 
{
	FILE *fp;
	char line[256];
	
	fp = fopen("/etc/mantle/metric_average_smoothers.config.txt", "r");
	while(fgets(line,256, fp))
	{
		if(strncmp("packets_per_minute_smoother: ", line, 29) == 0)
		{
			sscanf(line+29, "%lf", &packets_per_minute_smoother);
		}
		
		if(strncmp("cpu_load_average_smoother: ", line, 27) == 0)
		{
			sscanf(line+27, "%lf", &cpu_load_average_smoother);
		}
		
		if(strncmp("proportional_disk_activity_average_smoother: ", line, 45) == 0)
		{
			sscanf(line+45, "%lf", &proportional_disk_activity_average_smoother);
		}
		
		if(strncmp("proportional_used_memory_average_smoother: ", line, 43) == 0)
		{
			sscanf(line+43, "%lf", &proportional_used_memory_average_smoother);
		}
		
		if(strncmp("proportional_bandwidth_average_smoother: ", line, 41) == 0)
		{
			sscanf(line+41, "%lf", &proportional_bandwidth_average_smoother);
		}
	}
	
	fclose(fp);
	
}


static void save_data(struct sys_info *old_data, struct sys_info *new_data)
{
   //calculates cpu_load avaerage with cpu load average smoother  constant
    old_data->cpu_load = old_data->cpu_load * cpu_load_average_smoother + 
    (double)new_data->cpu_load * (1 - cpu_load_average_smoother);
    
    //calculates packets per minute with the packets per minute smoother constant
    old_data->packets_per_minute = old_data->packets_per_minute * packets_per_minute_smoother
    + ((1 - packets_per_minute_smoother) * (60.0 / (unix_time_now() - old_data->packet_time_stamp)));
    
    //updates packet time stamp
    //old_data->packet_time_stamp = unix_time_now();
    
     //calculates proportional free memory average
    old_data->proportional_used_mem = old_data->proportional_used_mem * proportional_used_memory_average_smoother +
    new_data->proportional_used_mem * (1.0 - proportional_used_memory_average_smoother);
    
    //calculates proportional disk activity average
    old_data->proportional_disk_activity = old_data->proportional_disk_activity * proportional_disk_activity_average_smoother +
    new_data->proportional_disk_activity * (1.0 - proportional_disk_activity_average_smoother);
    
    //calculates proportional bandwidth average
    old_data->proportional_bandwidth = old_data->proportional_bandwidth * proportional_bandwidth_average_smoother + 
    new_data->proportional_bandwidth * (1.0 - proportional_bandwidth_average_smoother);
    
    //possible bug fix
    old_data->packet_time_stamp = new_data->packet_time_stamp;
    

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
        
        if(numbytes != (sizeof(struct sys_info)))
        {
            sleep(1);
            continue;
        }
        
        //reads in from file what the metric average_smoother are
        //to be used in save_data
        find_metric_average_smoothers();
        
        
        //set time received
        new_packet->packet_time_stamp = unix_time_now();

        //saves information into old_data struct and simultaneously produces metrics
        save_data(&old_data, new_packet);
        

        //saves the information to the relevant machine specific struct 
        switch(new_packet->machine_type)
        {
            case (1):
                save_data(&old_br_data, new_packet);
                break;
            case (2):
                save_data(&old_ws_data, new_packet);
                break;
            case (3):
                save_data(&old_ds_data, new_packet);
                break;
            case (4):
                save_data(&old_as_data, new_packet);
                break;
               
        }
    
        FILE *fp;
        
        fp = fopen("/var/www/html/stored_sys_info.json", "w");
        
        fprintf(fp, "{\n \t \"cpu_load\" : %d  ,"
                  " \n \t \"proportional_used_mem\" : %lf ,"
                  " \n \t \"proportional_disk_activity\" : %lf ,"
                  " \n \t \"proportional_bandwidth\" : %lf ,"
                  " \n \t \"roles\" : [ "
                  " \n \t \t {\"role_id\" : 1 , "
                  " \n \t \t \"cpu_load\" : %d , "
                  " \n \t \t \"proportional_used_memory\" : %lf ,"
                  " \n \t \t \"proportional_disk_activity\" : %lf ,"
                  " \n \t \t \"proportional_bandwidth\" : %lf } ,\n"
                  " \n \t \t {\"role_id\" : 2 , "
                  " \n \t \t \"cpu_load\" : %d , "
                  " \n \t \t \"proportional_used_memory\" : %lf ,"
                  " \n \t \t \"proportional_disk_activity\" : %lf ,"
                  " \n \t \t \"proportional_bandwidth\" : %lf } ,\n"
                  " \n \t \t {\"role_id\" : 3 , "
                  " \n \t \t \"cpu_load\" : %d , "
                  " \n \t \t \"proportional_used_memory\" : %lf ,"
                  " \n \t \t \"proportional_disk_activity\" : %lf ,"
                  " \n \t \t \"proportional_bandwidth\" : %lf } ,\n"
                  " \n \t \t {\"role_id\" : 4 , "
                  " \n \t \t \"cpu_load\" : %d , "
                  " \n \t \t \"proportional_used_memory\" : %lf ,"
                  " \n \t \t \"proportional_disk_activity\" : %lf ,"
                  " \n \t \t \"proportional_bandwidth\" : %lf } "
                  " \n \t \t   ]"
                  "\n }", 
               (int)old_data.cpu_load, old_data.proportional_used_mem, //first elements of json string
               old_data.proportional_disk_activity, old_data.proportional_bandwidth,
               
               (int)old_br_data.cpu_load, old_br_data.proportional_used_mem, //batch robot fields
               old_br_data.proportional_disk_activity, old_br_data.proportional_bandwidth,
               
               (int)old_ws_data.cpu_load, old_ws_data.proportional_used_mem, //web server fields
               old_ws_data.proportional_disk_activity, old_ws_data.proportional_bandwidth,
               
               (int)old_ds_data.cpu_load, old_ds_data.proportional_used_mem, //database server fields
               old_ds_data.proportional_disk_activity, old_ds_data.proportional_bandwidth,
               
               (int)old_as_data.cpu_load, old_as_data.proportional_used_mem, //application server fields
               old_as_data.proportional_disk_activity, old_as_data.proportional_bandwidth);
        
        fclose(fp);
        
        
        
        

        printf("\nRecieved packet from IP address %s\n",
            inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s));
        printf("The Packet was %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        printf("The packet was sent by a: %d\n", new_packet->machine_type);
        printf("It was recieved at: %f \n", (double)new_packet->packet_time_stamp);
        printf("Packets arriving per minute is:  %f \n", old_data.packets_per_minute);
        printf("\nThe packet conatins: \n");
        printf("Instantaneous CPU Load: %d %%", (int)new_packet->cpu_load);
        printf("\nThe Free Memory on this machine was: %lf", (double)new_packet->free_mem);
        printf("\nThe Proportional Free Memory on this machine was: %lf %%", new_packet->proportional_used_mem);
        
        printf("\nAverage CPU load is: %d%% ", (int)old_data.cpu_load);
        
        printf("\nInstantaneous Disk activity was:  %d (reads/writes)", new_packet->disk_activity);
        printf("\nProportional Disk activity was: %lf %%", new_packet->proportional_disk_activity);
        printf("\nInstantaneous bandwidth was:  %lf bps)", new_packet->instantaneous_bandwidth);
        printf("\nProportional bandwidth was: %lf %% \n \n", new_packet->proportional_bandwidth);

    }
    close(sockfd);

    return 0;
}
