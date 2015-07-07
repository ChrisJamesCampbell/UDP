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
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//the struct that will conatin metrics which will be used
//to calculate state of system
struct sys_info     
{
    char cpu_load; //1 byte which represents CPU load average, with values between 0 and 100
    double free_mem;
    char machine_type;
    long disk_activity;
    long proportional_activity; //disk activity
    double instantaneous_bandwidth;
    int proportional_bandwidth;
    
    
    time_t packet_time_stamp;
    double packets_per_minute;
    
};


static void initialise_sys_info(struct sys_info *sys_info) 
{
    
    sys_info->cpu_load = 0;
    sys_info->free_mem = 0.0;
    sys_info->machine_type = 0;
    sysinfo->disk_activity = 0;
    sysinfo->proportional_activity = 0; //disk activity
    sysinfo->instantaneous_bandwidth = 0.0;
    sysinfo->proportional_bandwidth = 0;
    
    sys_info->packet_time_stamp = unix_time_now();
    sys_info->packets_per_minute = 0.0;
    return;
    
}


struct new_sys_info     
{
    char cpu_load; //1 byte which represents CPU load average, with values between 0 and 100
    double free_mem;
    char machine_type;
    long disk_activity;
    long proportional_activity; //disk activity
    double instantaneous_bandwidth;
    int proportional_bandwidth;
    time_t packet_time_stamp;
    double packets_per_minute;
};


static void initialise_new_sys_info(struct new_sys_info *new_sys_info) 
{
    
    new_sys_info->cpu_load = 0;
    new_sys_info->free_mem = 0.0;
    new_sys_info->machine_type = 0;
    new_sys_info->disk_activity = 0;
    new_sys_info->proportional_activity = 0; //disk activity
    new_sys_info->instantaneous_bandwidth = 0.0;
    new_sys_info->proportional_bandwidth = 0;
    new_sys_info->packet_time_stamp = unix_time_now();
    new_sys_info->packets_per_minute = 0.0;
    return;
    
}

static void save_data(struct sys_info *old_data, struct new_sys_info *new_data)
{
   //calculates cpu_load avaerage with cpu load average smoother
    old_data->cpu_load = old_data->cpu_load * CPU_LOAD__AVG_SMOOTHER + 
    (double)new_data->cpu_load * (1 - CPU_LOAD__AVG_SMOOTHER);
    
    //calculates packets per minute with the BETA constant smoother
    old_data->packets_per_minute = old_data->packets_per_minute * PACKETS_PM_SMOOTHER
    + (1 - PACKETS_PM_SMOOTHER) * (60 / (unix_time_now() - old_data->packet_time_stamp));
    
    //updates packet time stamp
    old_data->packet_time_stamp = unix_time_now();
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
    
   struct new_sys_info *new_packet = (struct new_sys_info *)buf;
   
   struct sys_info old_data;
   initialise_sys_info(&old_data);
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
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
        
        save_data(&old_data, new_packet);

        printf("Recieved packet from IP address %s\n",
            inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s));
        printf("The Packet was %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        printf("The Packet contains: \n[ \%d%% instantaneous CPU Load and %fKB Memory Free ]\n", 
        (int)new_packet->cpu_load, (double)new_packet->free_mem);
        printf("It was recieved at: %f \n", (double)new_packet->packet_time_stamp);
        
        printf("Packets arriving per minute is:  %f \n", old_data.packets_per_minute);
        
        printf("Average CPU load is: %d%% \n \n", (int)old_data.cpu_load);
        
        printf("\nInstantaneous Disk activity was:  %d (reads/writes)", new_packet->disk_activity);
        printf("\nProportional Disk activity was: %d %%", new_packet->proportional_activity);
        printf("\nInstantaneous bandwidth was:  %lf bps)", new_packet->instantaneous_bandwidth);
        printf("\nProportional bandwidth was: %d %% \n", new_packet->proportional_bandwidth);

    }
    close(sockfd);

    return 0;
}
