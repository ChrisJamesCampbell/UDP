#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	
	FILE *fp;
	char line[256];
	
	fp = fopen("/proc/meminfo","r");
	
	double receive_bytes = 0.0;
	double transmit_bytes = 0.0;
	
	
}
