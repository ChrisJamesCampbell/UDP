#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  FILE *fp;
	char line[256];
	int disk_writes1 = 0.0;
	int disk_writes2 = 0.0;
	int disk_writes3 = 0.0;
	
	int disk_reads1 = 0.0;
	int disk_reads2 = 0.0;
	int disk_reads3 = 0.0;
	int disk_activity = 0.0;
	
	
	fp = fopen("/proc/diskstats","r");
	
	
		while(fgets(line,256, fp))
		{
		
			if(strncmp("   8", line, 4) == 0)
			{
				//pulls the 'Reads Completed' successfully column for sda
				sscanf(line,"%*[ ]%*d%*[ ]%*d%*[ ]%*s%*[ ]%d" ,&disk_reads1);
				sscanf(line,"%*[ ]%*d%*[ ]%*d%*[ ]%*s%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%d", &disk_reads2);
				
			}
			
			
			
			/*if(count == 20)
			{
				fscanf(fp, "%d", &disk_reads);	//extract contents of line 8
			}*/
		
		}
		
	int disk_reads_total = disk_reads1 + disk_reads2 + disk_reads3;
	
	printf("The total number of disk reads was: %d \n", disk_reads_total);
	
	fclose(fp);
}
