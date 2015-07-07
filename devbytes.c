#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	

		
	while(1)
		
	{
		double received_bytes = 0.0;
		double transmitted_bytes = 0.0;
		
		double total_received_bytes = 0.0;
		double total_transmitted_bytes = 0.0;
		
		double network_activity[2];
		
		double bandwidth = 0.0;
		double proportional_bandwidth = 0.0;
		static double peak_bandwidth;
	
		
		int count = 1;
		
		FILE *fp;
		char line[256];
		
		
		
		while(count < 3)
			{
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
			
		    network_activity[count] = total_received_bytes + total_transmitted_bytes;
		    count++;
		    fclose(fp);
		    
		   if(count == 3)
		   {
		   	break;
		   }
		    
		   //resets total for next loop
		   total_received_bytes = 0.0;
		   total_transmitted_bytes = 0.0;
			
		   //sleep for one second in order to give us 
		   //network activity in terms of bytes/second
		sleep(1);
		}
		
		double relative_network_activity = network_activity[2] - network_activity[1];
		
		//converts the relative network activity given in bytes per second into BITS per second
		bandwidth = relative_network_activity * 8;
		
		if(bandwidth > peak_bandwidth)
		{
			peak_bandwidth = bandwidth;
		}
		
		if(peak_bandwidth > 0)
		{
			proportional_bandwidth = (bandwidth/peak_bandwidth);
		}
		
		printf("\n The total number of bytes received was: %lf", total_received_bytes);
		printf("\n The total number of bytes transmitted was: %lf", total_transmitted_bytes);
		printf("\n The relative network activity was: %lf Bytes", relative_network_activity);
		printf("\n The bandwidth was: %lf bits per second\n", bandwidth);
		
	
	
	}
	
	return 0;
	

}
