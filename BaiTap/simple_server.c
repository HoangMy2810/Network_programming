#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

int main()
{
    //socket(): khoi tao socket
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server == -1)
    {
        perror("socket() failed");
        exit(1);
    }
    else
    {
        printf("Socket created: %d\n", server);
    }


    //bind(): lien ket socket voi cau truc dia chi
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        exit(1);
    }
    

    //listen(): chuyen socket sang trang thai cho ket noi
    if (listen(server, 5))
    {
        perror("listen() failed");
        exit(1);
    }

    printf("Waiting for a new client ...\n");

    
    //accept(): chap nhan ket noi dang nam trong hang doi
    int client = accept(server, NULL, NULL);
    if (client == -1)
    {
        perror("accept() failed");
        exit(1);
    }
    else
    {
        printf("New client connected: %d\n", client);
    }


    //recv(): nhan du lieu tren socket
    char buf_recv[256];

        int ret_recv = recv(client, buf_recv, sizeof(buf_recv), 0);
        if (ret_recv <= 0)
        {
            perror("recv() failed");
        }
        puts(buf_recv);
    


    //send(): truyen du lieu tren socket
    char buf[256];
    for (int i = 0; i < 10; i++)
    {
        buf[i] = i;
    }
    printf("%s", buf);
    int ret = send(client, buf, strlen(buf), 0);
    if (ret == -1)
    {
        perror("send() failed");
        exit(1);
    }
    puts("Send successfully");
    close(client);
    close(server);    
    return 0;
}