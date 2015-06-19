//test to put some strain on the CPU 
//so that it its usage can be monitored

#include <stdio.h>
#include <math.h>
#include <stdlib.h.>

int main(void)
{
  printf("Start of test\n");
  long double i = rand() / (long double)RAND_MAX;
  
  for(int c = i; i<1000; c++)
  {
    long double a = i * i
  }
  
  printf("stop\n");

  return 0;

}
