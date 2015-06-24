#include <stdio.h>
#include <stdlib.h>

#define MEMFREE "MemFree:"
#define SHARED "Shared:"
#define CACHED "Cached:"

int main(void)
{
	
	FILE *fp;
	char line[256];
	
	double mem_free, shared, cached = 0.0;
	
	
	fp = fopen("/proc/meminfo","r");
	
	while(fgets(line,256, fp))
		{
			
			if(strncmp(MEMFREE, line, 8) == 0)
			{
				sscanf(line+8,"%*[ ]%lf", &mem_free);
			}
			
			if(strncmp(SHARED, line, 7) == 0)
			{
				sscanf(line+7,"%*[ ]%lf",&shared);
			}
			
			if(strncmp(CACHED, line, 7) == 0)
			{
				sscanf(line+7,"%*[ ]%lf", &cached);
			}
			
			
		}
		
		printf("Free Memory:%f Shared:%f Cached:%f \n", mem_free, shared, cached);

		fclose(fp);
		
		return 0;

}
