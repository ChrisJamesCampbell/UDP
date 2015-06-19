/*
** talker.c -- a datagram "client" demo
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

#define SERVERPORT "4950"    // the port users will be connecting to


//the struct that will conatin metrics which will be used
//to calculate state of system
struct sysinfo_type     
{
    char cpu_load; //1 byte which represents CPU load average, with values between 0 and 100
};


static void initialise_sysinfo(struct sysinfo_type *sysinfo) 
{
    
    sysinfo->cpu_load = 0;
    return;
    
    
    
}


//function that calculates the load average of the CPU
//and updates cpu_load within the struct sysinfo 
static void monitor_cpu_load(struct sysinfo_type *sysinfo) 
{
    long double newvalue[4], loadavg;
    static long double oldvalue[4];
    FILE *fp;
    char dump[50];

    
    //extrapolates CPU utilisation information
    fp = fopen("/proc/stat","r");
    fscanf(fp,"%*s %Lf %Lf %Lf %Lf",&newvalue[0],&newvalue[1],&newvalue[2],&newvalue[3]);
    fclose(fp);
    
    
    //calculates the load average of the cpu
    loadavg = ((oldvalue[0]+oldvalue[1]+oldvalue[2]) - (newvalue[0]+newvalue[1]+newvalue[2])) / 
    ((oldvalue[0]+oldvalue[1]+oldvalue[2]+oldvalue[3]) - (newvalue[0]+newvalue[1]+newvalue[2]+newvalue[3]));
    
    //replaces old value with most recent value
    oldvalue[0] = newvalue[0];
    
    
    //casting rounded double to char so as to only use 1 byte
    char cut_cpu_load = (char) (roundl(loadavg* 100));
    
    
  
    
    sysinfo->cpu_load = cut_cpu_load;
    return;
    
    
    
}



int main(int argc, char *argv[])
{
    
    while(1)
    {
    
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        
        struct sysinfo_type sysinfo;
        initialise_sysinfo(&sysinfo);
        
        
        
    
        if (argc != 2) {
            fprintf(stderr,"usage: talker hostname message\n");
            exit(1);
        }
    
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
    
        if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
    
        // loop through all the results and make a socket
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("talker: socket");
                continue;
            }
    
            break;
        }
    
        if (p == NULL) {
            fprintf(stderr, "talker: failed to bind socket\n");
            return 2;
        }
    
        if ((numbytes = sendto(sockfd, &sysinfo, sizeof(struct sysinfo_type), 0,
                 p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }
    
        freeaddrinfo(servinfo);
    
        printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
        close(sockfd);
    
        
        sleep(10);
    }
    return 0;
}
