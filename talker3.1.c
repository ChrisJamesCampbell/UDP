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

/*#define BATCH_ROBOT 1
#define WEB_SERVER 2
#define DATABASE_SERVER 3
#define APPLICATION_SERVER 4*/

//a signal counter which will be incremented at the end of the
//main method to signal that the program has run more than once
static int first_time = 0;

//the struct that will conatin metrics which will be used
//to calculate state of system
struct sys_info_type     
{
    char cpu_load; //1 byte which represents CPU load average, with values between 0 and 100
    double free_mem;
    double proportional_free_mem;
    int machine_type;
    long disk_activity;
    double proportional_disk_activity; 
    double instantaneous_bandwidth;
    double proportional_bandwidth;
    
};


static void initialise_sys_info(struct sys_info_type  sys_info) 
{
    
    sys_info->cpu_load = 0;
    sys_info->free_mem = 0.0;
    sys_info->proportional_free_mem = 0.0;
    sys_info->machine_type = 0;
    sys_info->disk_activity = 0;
    sys_info->proportional_disk_activity = 0.0;
    sys_info->instantaneous_bandwidth = 0.0;
    sys_info->proportional_bandwidth = 0.0;
    return;
    
}


//function that calculates the load average of the CPU
//and updates cpu_load within the struct sys_info 
static void find_cpu_load(struct sys_info_type  sys_info) 
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
    sys_info->cpu_load = cut_cpu_load;
    return;
    
}

static void find_free_memory(struct sys_info_type  sys_info)
{
    FILE *fp;
	char line[256];
	
	double mem_total, mem_free, buffers, cached = 0.0;
	double actual_mem_free = 0.0;
	
	
	fp = fopen("/proc/meminfo","r");
	
	//scans file by looking at each indiviudal line which ends at the 256th character of each line
	while(fgets(line,256, fp))
		{
			//if the first 8 characters on the line match this string
			if(strncmp("MemFree:", line, 8) == 0)
			{
				//ignore the following white space and scan in the float and save it
				sscanf(line+8,"%*[ ]%lf", &mem_free);
			}
			
			if(strncmp("MemTotal:", line, 9) == 0)
			{
				sscanf(line+9, "%*[ ]%lf", &mem_total);
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
		
		//gives us the real total available free memory
		actual_mem_free = mem_free + buffers + cached; 
		
	 sys_info->proportional_free_mem = (actual_mem_free / mem_total) * 100;
		
		//update the free_mem double within sys_info struct
	 sys_info->free_mem = actual_mem_free; 
		
		fclose(fp);
}

static void find_disk_info(struct sys_info_type  sys_info)
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
	
	//iterate over every line in the file
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
	 sys_info->disk_activity = (new_disk_activity - old_disk_activity);
	}
	
	//stores the most recently read disk activity in the static old disk activity
	//so we can use it on the next call to this method to find what the difference has 
	//been in disk activity
	old_disk_activity = new_disk_activity;
	
	//as soon as any activity has been monitored, highest activity
	//will be set as the first reading and then after that will only
	//be replaced if the last recorded activity is greater than the highest recorded activity
	if sys_info->disk_activity > highest_activity)
	{	
		highest_activity = sys_info->disk_activity;
	
	}
	
	//proportional activity is the proportion of the monitored activity
	//over the highest recorded activity so far
	if(highest_activity > 0)
	{
	 sys_info->proportional_disk_activity =   sys_info->disk_activity/ (double)highest_activity) * 100;
	}
	
}

static void find_bandwidth(struct sys_info_type  sys_info)
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
		if(strncmp("lo", line, 2) == 0)
		{
			continue;
		}
					
		//pulls the receieved bytes information which is the first column of integers
		//within /proc/net/dev
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
		//converts the network activity given to us in bytes into BITS per second
		//(divide by 5 since currently sending packets every 5 seconds)
	 sys_info->instantaneous_bandwidth = ((new_network_activity - old_network_activity) * 8) / 5;
		
    }
		
	//stores the network activity JUST read in this call into the static double
	//old network activity so we can use the value to find the difference in total activity
	//between calls to this method
	old_network_activity = new_network_activity;
	
	if sys_info->instantaneous_bandwidth > peak_bandwidth)
	{
		peak_bandwidth = sys_info->instantaneous_bandwidth;
	}
	
	if(peak_bandwidth > 0)
	{
		//gives the proportional bandwidth of the instantaneous bandwidth in 
		//terms of percentage of the known peak bandwidth
	 sys_info->proportional_bandwidth = ( sys_info->instantaneous_bandwidth/peak_bandwidth) * 100);
	}
}

static void what_machine_type(struct sys_info_type  sys_info)
{
	     //opens the file which will tell us what kind of machine is
	//sending the packet
	FILE *fp2;
	char line2[256];
	
	fopen("/etc/mantle/role", "r");
	
	while(fgets(line2,256, fp2))
	{
		if(strncmp("robot", line2, 5) == 0)
		{
		 sys_info->machine_type = 1;
			break;
		}
		
		if(strncmp("web", line2, 3) == 0)
		{
		 sys_info->machine_type = 2;
			break;
		}
		
		if(strncmp("galera", line2, 6) == 0)
		{
		 sys_info->machine_type = 3;
			break;
		}
		
		if(strncmp("fcgi", line2, 4) == 0)
		{
		 sys_info->machine_type = 4;
			break;
		}
	}
	
	fclose(fp2);
}

int main()
{ 

    while(1)
    {
    
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        
        struct sys_info_type sys_info;
        initialise_sys_info( sys_info);
        
        //calls the methods to extrapolate the metrics
        find_cpu_load( sys_info);
        find_free_memory( sys_info);
        find_disk_info( sys_info);
        find_bandwidth( sys_info);
        
        what_machine_type( sys_info);
        
        FILE *fp;
        char line[256];
        char ip_to_send[40];
		
	    fp = fopen("/root/UDP/ip_addresses.txt","r");
	
	    while(fgets(line,256, fp))
	    {
	    	sscanf(line, "%s", ip_to_send);
	    	memset(&hints, 0, sizeof hints);
	    	
	        hints.ai_family = AF_UNSPEC;
	        hints.ai_socktype = SOCK_DGRAM;
	    
	        if ((rv = getaddrinfo(ip_to_send, SERVERPORT, &hints, &servinfo)) != 0) 
	        {
	            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	            return 1;
	        }
	    
	        // loop through all the results and make a socket
	        for(p = servinfo; p != NULL; p = p->ai_next) 
	        {
	            if ((sockfd = socket(p->ai_family, p->ai_socktype,
	               p->ai_protocol)) == -1)
	                    {
	                		perror("talker: socket");
	                		continue;
	            		}
	            break;
	        }
	    
	        if (p == NULL) {
	            fprintf(stderr, "talker: failed to bind socket\n");
	            return 2;
	        }
	        
	         if ((numbytes = sendto(sockfd,  sys_info, sizeof(struct sys_info_type), 0,
             p->ai_addr, p->ai_addrlen)) == -1) 
	         {
	            perror("talker: sendto");
	            exit(1);
	         }
	    
	    } //end of reading ip addresses/making socket loop
	    
		freeaddrinfo(servinfo);
	        
	    fclose(fp);
        
        printf("\nTalker: sent %d bytes to %s", numbytes, ip_to_send);
        printf("\nThe machine which sent the packet was of type: %d", sys_info.machine_type);
        printf("\nInsantaneous CPU load was: %d %%", sys_info.cpu_load);
        printf("\nFree memory on this machine is: %lf KB", sys_info.free_mem);
        printf("\nProportional free memory on this machine is: %lf %%", sys_info.proportional_free_mem);
        printf("\nInstantaneous Disk activity was:  %d (reads/writes)", sys_info.disk_activity);
        printf("\nProportional Disk activity was: %lf %%", sys_info.proportional_disk_activity);
        printf("\nInstantaneous bandwidth was:  %lf bps)", sys_info.instantaneous_bandwidth);
        printf("\nProportional bandwidth was: %lf %% \n", sys_info.proportional_bandwidth);
        
        close(sockfd);
       
        sleep(5);
        
        //increments the signal counter to say that the program has been 
        //run at least once
        first_time++;
    }
    return 0;
}
