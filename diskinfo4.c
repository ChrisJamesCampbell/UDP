#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  FILE *fp;
	char line[256];
	
	int disk_reads;
	int disk_reads_total;
	
	int disk_writes;
	int disk_writes_total;
	
	int disk_activity = 0.0;
	
	
	fp = fopen("/proc/diskstats","r");
	
	
		while(fgets(line,256, fp))
		{
		
			if(strncmp("   8", line, 4) == 0)
				//need to insert some form of while() loop
				//in order to add up all reads and writes across all sda's
				while()
				{
					//pulls the 'Reads Completed' successfully column for an sda and adds to total
					sscanf(line,"%*[ ]%*d%*[ ]%*d%*[ ]%*s%*[ ]%d" ,&disk_reads);
					disk_reads_total = disk_reads_total + disk_reads;
					
					//pulls the 'Writes Completed' successfully column for an sda and adds to total
					sscanf(line,"%*[ ]%*d%*[ ]%*d%*[ ]%*s%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%d", &disk_writes);
					disk_writes_total = disk_writes_total + disk_writes;
				}
			
			
		}
		
	disk_activity = disk_reads_total + disk_writes_total;
	
	printf("\nThe total number of disk reads was: %d \n", disk_reads_total);
	printf("The total number of disk writes was: %d \n", disk_writes_total);
	printf("The total disk activity was: %d \n \n", disk_activity);
	
	fclose(fp);
}
