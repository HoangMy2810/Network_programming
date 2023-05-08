#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9000);
    int ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("connect() failed");
        return 1;
    }

    char *buf_send = "Hello server from client";
    if(send(client, buf_send, strlen(buf_send), 0) == -1)
    {
        perror("send() failed");
        return 1;
    }

    char buf[256];
        int ret_recv = recv(client, buf, sizeof(buf), 0);
        // if (ret_recv <= 0)
        // {
        //     break;
        // }
        //buf = strcat(buf, " Hello");
        printf("Data reveiced: %s Hello", buf);
    close(client);
    return 0;
}
