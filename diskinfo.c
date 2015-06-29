#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  FILE *fp;
	char line[256];
	int disk_writes = 0.0;
	int disk_reads = 0.0;
	int disk_activity = 0.0;
	int count = 0;
	
	
	fp = fopen("/proc/diskstats","r");
	
	
		while(fgets(line,256, fp))
		{
			if(strncmp("8", line, 1) == 0)
			{
				sscanf(line+12, "%*[ ]%d", &disk_writes);//sadfgad
			}
			
			if(count == 8)
			{
				fscanf(fp, "%d", &disk_reads);	//extract contents of line 8
			}
		count++;
		}
		
	disk_activity = disk_writes + disk_reads;
	
	printf("The disk activity was: %d \n", disk_activity);
	
	fclose(fp);
}
