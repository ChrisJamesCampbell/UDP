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
	   sleep(5);
	
	}
		
	relative_activity = disk_activity[2] - disk_activity[1];
	
	printf("\nThe total number of disk reads was: %d \n", disk_reads_total);
	printf("The total number of disk writes was: %d \n", disk_writes_total);
	printf("The relative disk activity was: %d \n", relative_activity);

}
