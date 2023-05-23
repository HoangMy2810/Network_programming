#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_LEN 20

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <IP-address> <dist-port> <your-port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr(argv[1]);
    send_addr.sin_port = htons(atoi(argv[2]));

    int receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in recei_addr;
    memset(&send_addr, 0, sizeof(recei_addr));
    recei_addr.sin_family = AF_INET;
    recei_addr.sin_addr.s_addr = INADDR_ANY;
    recei_addr.sin_port = htons(atoi(argv[3]));

    bind(receiver, (struct sockaddr *)&recei_addr, sizeof(recei_addr));

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(STDIN_FILENO, &fdread);
    FD_SET(receiver, &fdread);
    char buf[256];
    while(1)
    {
        fdtest = fdread;
        int ret = select(receiver + 1, &fdtest, NULL, NULL, NULL);
        if(ret < 0)
        {
            perror("select() failed");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdtest))
        {
            fgets(buf, sizeof(buf), stdin);
            sendto(server, buf, strlen(buf), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
        }

        if(FD_ISSET(receiver, &fdtest))
        {
            ret = recvfrom(receiver, buf, sizeof(buf),0, NULL, NULL);
            buf[ret] = 0;
            printf("Received: %s\n", buf);
        }
    }
    close(server);
    close(receiver);
    return 1;
}