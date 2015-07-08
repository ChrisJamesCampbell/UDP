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

//a signal counter which will be incremented at the end of the
//main method to signal that the program has run more than once
static int first_time = 0;

//the struct that will conatin metrics which will be used
//to calculate state of system
struct sysinfo_type     
{
    char cpu_load; //1 byte which represents CPU load average, with values between 0 and 100
    double free_mem;
    char machine_type;
    long disk_activity;
    double proportional_activity; //disk activity
    double instantaneous_bandwidth;
    double proportional_bandwidth;
};


static void initialise_sysinfo(struct sysinfo_type *sysinfo) 
{
    
    sysinfo->cpu_load = 0;
    sysinfo->free_mem = 0.0;
    sysinfo->machine_type = 0;
    sysinfo->disk_activity = 0;
    sysinfo->proportional_activity = 0.0;
    sysinfo->instantaneous_bandwidth = 0.0;
    sysinfo->proportional_bandwidth = 0.0;
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
	
	long disk_reads = 0;
	long disk_reads_total = 0;
	
	long disk_writes = 0;
	long disk_writes_total = 0;
	
	static long old_disk_activity;
	long new_disk_activity = 0;

	static long highest_activity;


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
		
		fclose(fp);
	    new_disk_activity = disk_reads_total + disk_writes_total;
	    
	 
	
	//if the program has run at least once, update disk_activity
	//(avoids erroneous disk activity results by missing first reading)
	if(first_time > 0)
	{
		sysinfo->disk_activity = (new_disk_activity - old_disk_activity);
	}
	
	//stores the most recently read disk activity in the static old disk activity
	//so we can use it on the next call to this method to find what the difference has 
	//been in disk activity
	old_disk_activity = new_disk_activity;
	
	//as soon as relative activity has been monitored, highest activity
	//will be set as the first reading and then after that will only
	//be replaced if the relative activity is greater than the highest recorded activity
	if(sysinfo->disk_activity > highest_activity)
	{	
		highest_activity = sysinfo->disk_activity;
	
	}
	
	//proportional activity is the proportion of the relative activity
	//against the highest recorded activity so far
	if(highest_activity > 0)
	{
		sysinfo->proportional_activity =  (sysinfo->disk_activity/ (double)highest_activity) * 100;
	}
	
	

}

static void find_bandwidth(struct sysinfo_type *sysinfo)
{
	    double received_bytes = 0.0; //holds the value of each received bytes for each row 
		double transmitted_bytes = 0.0; //holds the value of each transmitted bytes for each row 
		
		double total_received_bytes = 0.0; //holds the total of the recieved bytes column
		double total_transmitted_bytes = 0.0; //holds the total of the transmitted bytes column
		
		static double old_network_activity;
		double new_network_activity = 0.0;

		//keeps a record of the highest recorded bandwidth
		static double peak_bandwidth;
		
	

		FILE *fp;
		char line[256];
		
		
		
			    fp = fopen("/proc/net/dev","r");
			    
				while(fgets(line,256, fp))
				{
					
					//pulls the receieved bytes information which is the first column of integers
					//within net/dev
					sscanf(line, "%*[ ]%*s%*[ ]%lf", &received_bytes);
					total_received_bytes = total_received_bytes + received_bytes;
					
					
					//pulls the transmitted bytes information which is represented by the 9th column of 
					//integers within net/dev
					sscanf(line, "%*[ ]%*s%*[ ]%*lf%*[ ]%*lf%*[ ]%*lf%*[ ]%*lf%*[ ]%*lf%*[ ]%*lf%*[ ]%*lf%*[ ]%*lf%*[ ]%lf",
					&transmitted_bytes);
					total_transmitted_bytes = total_transmitted_bytes + transmitted_bytes;
					
					
				}
			
			fclose(fp);
		    new_network_activity = total_received_bytes + total_transmitted_bytes;
		   
		   //if the prgram has been run more than once,update instantaneous bandwidth.
		   //we do this to account for the fact the first reading would produce erroneous results
		   //and so bandwidth results are only accurate after the second run of this method
		   if(first_time > 0)
		  	{
		
				//takes the difference between network activity and 
				//converts the network activity given to us in bytes into BITS
				sysinfo->instantaneous_bandwidth = (new_network_activity - old_network_activity) * 8;
				
		  	}
		
		//stores the network activity JUST read in this call into the static double
		//old network activity so we can use the value to find the difference in total activity
		//between calls to this method
		old_network_activity = new_network_activity;
		
		if(sysinfo->instantaneous_bandwidth > peak_bandwidth)
		{
			peak_bandwidth = sysinfo->instantaneous_bandwidth;
		}
		
		if(peak_bandwidth > 0)
		{
			//gives the proportional bandwidth of the instantaneous bandwidth in 
			//terms of percentage of the known peak bandwidth
			sysinfo->proportional_bandwidth = ((sysinfo->instantaneous_bandwidth/peak_bandwidth) * 100);
		}
}
 /*
static void find_ip_addresses()
{
	FILE *fp;
	char line[256];
	char ip_address_list[5][40]; //max ipv6 address is 39 characters long, ipv4 is 19
	int i = 1;
	
	fp = fopen("/UDP/ip_addresses.txt","r");
	
	while(fgets(line,256, fp))
	{
		for(i =1; i < 6; i++ )
		{
			sscanf(line, "%s", ip_address_list[i]);
			printf("IP address %d is: %s", ip_address_list[i]);
		}
	}
}
*/


int main()
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
        find_bandwidth(&sysinfo);
        
    
        /*if (argc != 2) {
            fprintf(stderr,"usage: talker hostname message\n");
            exit(1);
        }*/
        
        FILE *fp;
		char line[256];
		char ip_to_send[9];
		
		fp = fopen("/UDP/ip_addresses.txt","r");
	
	    while(fgets(line,256, fp))
	    {
	    	sscanf(line, "%s", ip_to_send);
	    	memset(&hints, 0, sizeof hints);
	    	
	        hints.ai_family = AF_UNSPEC;
	        hints.ai_socktype = SOCK_DGRAM;
	    
	        if ((rv = getaddrinfo(ip_to_send, SERVERPORT, &hints, &servinfo)) != 0) {
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
	        
	        fclose(fp);
	    

	    }
	    
	         if ((numbytes = sendto(sockfd, &sysinfo, sizeof(struct sysinfo_type), 0,
             p->ai_addr, p->ai_addrlen)) == -1) 
	         {
	            perror("talker: sendto");
	            exit(1);
	         }
	    
	        freeaddrinfo(servinfo);
    
        
    
        /*printf("talker: sent %d bytes to %s containing %d and %f and %d and %d and %lf\n", numbytes, argv[1], 
        sysinfo.cpu_load, sysinfo.free_mem, sysinfo.disk_activity, sysinfo.proportional_activity,
        sysinfo.proportional_bandwidth);
        close(sockfd);*/
        
        printf("\nTalker: sent %d bytes to %s", numbytes, "127.0.0.1");
        printf("\nInsantaneous CPU load was: %d %%", sysinfo.cpu_load);
        printf("\nFree memory on this machine is: %f KB", sysinfo.free_mem);
        printf("\nInstantaneous Disk activity was:  %d (reads/writes)", sysinfo.disk_activity);
        printf("\nProportional Disk activity was: %lf %%", sysinfo.proportional_activity);
        printf("\nInstantaneous bandwidth was:  %lf bps)", sysinfo.instantaneous_bandwidth);
        printf("\nProportional bandwidth was: %lf %% \n", sysinfo.proportional_bandwidth);
        
        close(sockfd);
       
        sleep(5);
        
        //increments the signal counter to say that the program has been 
        //run at least once
        first_time++;
    }
    return 0;
}
