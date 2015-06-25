#include <stdlib.h>
#include <string.h>
#define UNIX 1

//remove the above line if running under Windows

#ifdef UNIX
    #include <unistd.h>
#else
    #include <windows.h>
#endif

int main(int argc, char** argv)
{
    unsigned long mem;
    if(argc==1)
        mem = 1024*1024*512; //512 mb
    else if(argc==2)
        mem = (unsigned) atol(argv[1]);
    else
    {
        printf("Usage: loadmem <memory in bytes>");
        exit(1);
    }

    char* ptr = malloc(mem);
    while(1)
    {
        memset(ptr, 0, mem);
        #ifdef UNIX
            sleep(120);
        #else
            Sleep(120*1000);
        #endif
    }
}
