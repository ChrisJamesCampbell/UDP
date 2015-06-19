//test to put some strain on the CPU 
//so that it its usage can be monitored

#include <stdio.h>
#include <math.h>

int main(void)
{
  printf("Start of test\n");
  long double a = rand();
  long double b = rand();
  long double i = 0.0;
  
  if(a > b)
    {
        i = a;
    }
  else
    {
        i = b;
    }
  
  for(int c = 0; c<10000000; c++)
  {
    long double a = i * i;
  }
  
  printf("End of test\n");

  return 0;

}
