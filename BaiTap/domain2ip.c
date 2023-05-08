#include <stdio.h>
#include <sys/socket.h> //thu vien lam viec voi socket
#include <netdb.h> // lam viec voi dia chi mang va ten mien
#include <arpa/inet.h> //lam viec voi dia chi IP (ipV4 --> in_addr,...)

int main()
{
    struct addrinfo *result, *p, hints;
    hints.ai_family = AF_INET6; //AF_INET: IPv4
    int ret = getaddrinfo("google.com", "http", &hints, &result); //tenmien -> dia chi mang IPv4)
    if (ret != 0)
    {
        printf("Failed to get IP: %s\n", gai_strerror(ret));
        return 1;
    }

    printf("Success to get IP.\n");

    p = result;
    while (p != NULL)
    {
        if (p->ai_family == AF_INET)
        {
            printf("IPv4: %s \n",
                inet_ntoa(((struct sockaddr_in *)p->ai_addr)->sin_addr)); //chuyen doi dia chi IPv4 tu decimal -> string
        }
        else if (p->ai_family == AF_INET6)
        {
            char buf[64];
            printf("IPv6: %s\n", inet_ntop(AF_INET6, 
                &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr,
                buf, INET6_ADDRSTRLEN));
            //printf("%s", buf);
        }
        p = p->ai_next;
    }
    return 0;
}