int main()
{
  FILE *fp;
	char line[256];
	double disk_writes = 0.0;
	double disk_reads = 0.0;
	int count = 0;
	
	
	fp = fopen("/proc/diskstats","r");
	
	
		while(fgets(line,256, fp))
		{
			if(count == 4)
			{
				fscanf(fp, "%f", disk_writes);//extract contents of line 4
			}
			
			if(count == 8)
			{
				fscanf(fp, "%f", disk_reads);	//extract contents of line 8
			}
		count++;
		}
		
	sysinfo->disk_activity = disk_writes + disk_reads;
	
	fclose(fp);
}
