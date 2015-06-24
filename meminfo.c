#include <stdio.h>
#include <stdlib.h>

#define MEMFREE "MemFree:"
#define BUFFERS "Buffers:"
#define CACHED "Cached:"

int main(void)
{
	
	FILE *fp;
	char line[256];
	
	double mem_free, buffers, cached = 0.0;
	
	
	fp = fopen("/proc/meminfo","r");
	
	while(fgets(line,256, fp))
		{
			
			if(strncmp(MEMFREE, line, 8) == 0)
			{
				sscanf(line+8,"%*[ ]%lf", &mem_free);
			}
			
			if(strncmp(BUFFERS, line, 8) == 0)
			{
				sscanf(line+8,"%*[ ]%lf",&buffers);
			}
			
			if(strncmp(CACHED, line, 7) == 0)
			{
				sscanf(line+7,"%*[ ]%lf", &cached);
			}
			
			
		}
		
		printf("Free Memory:%f Buffers:%f Cached:%f \n", mem_free, buffers, cached);

		fclose(fp);
		
		return 0;

}
