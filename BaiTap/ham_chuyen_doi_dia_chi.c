#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    /*
    -------IP dang xau sang so nguyen 32 bit-------
    const char* cp = "127.0.0.";
    in_addr_t res = inet_addr(cp); 
    printf("%u\n", res);*/
    
    
    /*
    -------IP dang xau <=> cau truc in_addr--------
    const char* cp = "127.0.0.1";
    struct in_addr inp;
    if (inet_aton(cp, &inp) != 0) //xau -> in_addr
    {
        printf("Success\n");
        printf("%s\n", inet_ntoa(inp)); //in_addr -> xau
    }
    else
    {
        printf("Invalid IPv4 address");
        exit(EXIT_FAILURE);
    }*/


    return 0;
}