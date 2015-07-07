#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	
	double received_bytes = 0.0;
	double transmitted_bytes = 0.0;
	
	double total_received_bytes = 0.0;
	double total_transmitted_bytes = 0.0;
	
	double network_activity[2];
	
	
	
	
	int count = 1;
	
	FILE *fp;
	char line[256];
	
	fp = fopen("/proc/net/dev","r");
	
	
	while(count < 3)
		{
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
		
		sleep(1);
	}
	
	double relative_network_activity = network_activity[2] - network_activity[1];
	
	printf("\n The total number of bytes received was: %lf", total_received_bytes);
	printf("\n The total number of bytes transmitted was: %lf\n", total_transmitted_bytes);
	
	
	
	
	
}
