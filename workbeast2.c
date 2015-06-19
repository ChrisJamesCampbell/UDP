//test to put some strain on the CPU 
//so that it its usage can be monitored

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main(void)
{
  printf("Start of test\n");
  /*long double a = rand();
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
  */
  
  long double a = 0;
  int i = rand();
  
  for(int c = 0; c<1000000000000; c++)
  {
    a = i * i;
    printf(a);
  }
  
  printf("End of test\n");

  return 0;

}
