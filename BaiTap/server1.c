#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr; //bieu dien dia chi IP & cong ket noi
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000); 

    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr))) //gan dia chi cho socket, socket ban dau ko co dia chi
    {
        perror("bind() failed");
        return 1;
    }

    if(listen(listener, 5)) //listen(socket da duoc gan dia chi, so luong toi da ket noi cho doi)
    {
        perror("listen() failed");
        return 1;
    }

    printf("Khoi tao thanh cong!!!\n");

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("Accepted socket %d from IP: %s:%d\n", 
        client,
        inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port));

    char buf[256];    
    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret < sizeof(buf))
        buf[ret] = 0;
    printf("%d bytes from client: %s", ret, buf);
    
    while (1)
    {
        printf("Enter string: ");
        fgets(buf, sizeof(buf), stdin);
        send(client, buf, strlen(buf), 0);

        if (strncmp(buf, "exit", 4) == 0)
            break;
    }
    
    close(client);
    close(listener);
}