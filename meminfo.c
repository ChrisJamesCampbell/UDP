#include <stdio.h>
#include <stdlib.h>

#define MEMTOTAL "MemTotal:"
#define SHARED "Shared:"
#define CACHED "Cached:"

int main(void)
{
	
	FILE *fp;
	char line[256];
	
	double mem_total, shared, cached = 0.0;
	
	
	fp = fopen("/proc/meminfo","r");
	
	while(fgets(line,256, fp))
		{
			
			if(strncmp(MEMTOTAL, line, 9) == 0)
			{
				sscanf(line, "%s*[ ] %f", &mem_total);
			}
			
			if(strncmp(SHARED, line, 7) == 0)
			{
				sscanf(line, "%s*[ ] %f", &shared);
			}
			
			if(strncmp(CACHED, line, 7) == 0)
			{
				sscanf(line, "%s*[ ] %f", &cached);
			}
		}
		
		printf("%f %f %f", mem_total, shared, cached);
		
		fclose(fp);
		
		return 0;

}
