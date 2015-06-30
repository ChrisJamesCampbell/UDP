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

#define BATCH_ROBOT "0"
#define WEB_SERVER "1"
#define DATABASE_SERVER "2"
#define APPLICATION_SERVER "3"

//the struct that will conatin metrics which will be used
//to calculate state of system
struct sysinfo_type     
{
    char cpu_load; //1 byte which represents CPU load average, with values between 0 and 100
    double free_mem;
    char machine_type;
    long disk_activity;
};


static void initialise_sysinfo(struct sysinfo_type *sysinfo) 
{
    
    sysinfo->cpu_load = 0;
    sysinfo->free_mem = 0.0;
    sysinfo->machine_type = 0;
    sysinfo->disk_activity = 0;
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
    loadavg = ((newvalue[0]+newvalue[1]+newvalue[2]) - (oldvalue[0]+oldvalue[1]+oldvalue[2])) / 
    ((newvalue[0]+newvalue[1]+newvalue[2]+newvalue[3]) - (oldvalue[0]+oldvalue[1]+oldvalue[2]+oldvalue[3]));
    
    //replaces old value with most recent value
    //across the entire array
    oldvalue[0] = newvalue[0];
    oldvalue[1] = newvalue[1];
    oldvalue[2] = newvalue[2];
    oldvalue[3] = newvalue[3];
    
    
    //casting rounded double to char so as to only use 1 byte
    char cut_cpu_load = (char) (roundl(loadavg* 100));
    
    
  
    
    sysinfo->cpu_load = cut_cpu_load;
    return;
    
    
    
}

static void find_free_memory(struct sysinfo_type *sysinfo)
{
    FILE *fp;
	char line[256];
	
	double mem_free, buffers, cached = 0.0;
	double actual_mem_free = 0.0;
	
	
	fp = fopen("/proc/meminfo","r");
	
	//scans file by looking at each indiviudal line which ends at the 256th character of each line
	while(fgets(line,256, fp))
		{
			
			if(strncmp("MemFree:", line, 8) == 0)
			{
				sscanf(line+8,"%*[ ]%lf", &mem_free);
			}
			
			if(strncmp("Buffers:", line, 8) == 0)
			{
				sscanf(line+8,"%*[ ]%lf",&buffers);
			}
			
			if(strncmp("Cached:", line, 7) == 0)
			{
				sscanf(line+7,"%*[ ]%lf", &cached);
			}
			
			
		}
		
		actual_mem_free = mem_free + buffers + cached; //gives us the real total available free memory
		
		sysinfo->free_mem = actual_mem_free; //update the free_mem double within sysinfo struct
		
		//printf("Free Memory:%f Buffers:%f Cached:%f \n", mem_free, buffers, cached);

		fclose(fp);
}

static void find_disk_info(struct sysinfo_type *sysinfo)
{
  	FILE *fp;
	char line[256];
	
	int disk_reads;
	int disk_reads_total;
	
	int disk_writes;
	int disk_writes_total;
	int count = 1;
	
	int disk_activity[2];
	
	int relative_activity = 0;
	
	
	
	while(count < 3)
	{
	    fp = fopen("/proc/diskstats","r");
	
	    while(fgets(line,256, fp))
		
		{
			//pulls the 'Reads Completed' successfully column for an sda and adds to total
			sscanf(line,"%*[ ]%*d%*[ ]%*d%*[ ]%*s%*[ ]%d" ,&disk_reads);
			disk_reads_total = disk_reads_total + disk_reads;
			
			//pulls the 'Writes Completed' successfully column for an sda and adds to total
			sscanf(line,"%*[ ]%*d%*[ ]%*d%*[ ]%*s%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%d", &disk_writes);
			disk_writes_total = disk_writes_total + disk_writes;
			
	
		}
		
	   disk_activity[count] = disk_reads_total + disk_writes_total;
	   count++;
	   fclose(fp);

	}
		
	relative_activity = disk_activity[2] - disk_activity[1];
	sysinfo->disk_activity = relative_activity - sysinfo->disk_activity;

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
        
        //calls the methods to extrapolate the metrics
        monitor_cpu_load(&sysinfo);
        find_free_memory(&sysinfo);
        find_disk_info(&sysinfo);
        
    
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
    
        printf("talker: sent %d bytes to %s containing %d and %f and %d\n", numbytes, argv[1], 
        sysinfo.cpu_load, sysinfo.free_mem, sysinfo.disk_activity);
        close(sockfd);
    
        
        sleep(10);
    }
    return 0;
}
