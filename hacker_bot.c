/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define SERVERPORT "4950"    // the port users will be connecting to
//static char skidouche = " ";

/*static char *rand_string(char *str, long limit)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    int n;
    
        for (n = 0; n < limit; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[limit] = '\0';
    
    return str;
}*/

char *randstring(size_t length) {

    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";        
    char *randomString = NULL;
    int n;
    

    if (length) {
        randomString = malloc(sizeof(char) * (length +1));

        if (randomString) {            
            for (n = 0;n < length;n++) {            
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }

            randomString[length] = '\0';
        }
    }

    return randomString;
}


	// Assumes 0 <= max <= RAND_MAX
	// Returns in the half-open interval [0, max]
	  static long random_at_most(long limit) {
	  unsigned long
	    // max <= RAND_MAX < ULONG_MAX, so this is okay.
	    num_bins = (unsigned long) limit + 1,
	    num_rand = (unsigned long) RAND_MAX + 1,
	    bin_size = num_rand / num_bins,
	    defect   = num_rand % num_bins;
	
	  long x;
	  do {
	   x = random();
	  }
	  // This is carefully written not to overflow
	  while (num_rand - defect <= (unsigned long)x);
	
	  // Truncated division is intentional
	  return x/bin_size;
}


int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    long limit = 512;
    
    while(1)
    {
    int numbytes = (int) rand();
    
    if(numbytes < 512)
    {
    	break;
    }
    }
    //random_at_most(limit);
    //char* random_string = *rand_string(&skidouche, limit);
    
    char *random_string = randstring(limit);
    

    if (argc != 2) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }
    
    while(1)
    {
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }

    /*if ((strlen(random_string) = sendto(sockfd, random_string, strlen(random_string), 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }*/

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", strlen(random_string), argv[1]);
    printf("The string contained: %s \n", random_string);
    close(sockfd);
    }

    return 0;
}
