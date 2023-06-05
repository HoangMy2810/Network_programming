#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_BUF_SIZE 1024
#define NUMBER_PROCESSES 8
#define NUMBER_CLIENTS 10

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server, NUMBER_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    
    char buf[MAX_BUF_SIZE];

    for (int i = 0; i < NUMBER_PROCESSES; i++)
    {
        if (fork() == 0)
        {
            while (1)
            {
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_addr_len = sizeof(client_addr);
                int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client < 0)
                {
                    perror("accept() failed");
                    exit(EXIT_FAILURE);
                }
                int len = recv(client, buf, MAX_BUF_SIZE, 0);
                if (len < 0)
                {
                    perror("recv() failed");
                    exit(EXIT_FAILURE);
                }
                else if (len == 0)
                {
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    close(client);
                    continue;
                }
                else
                {
                    buf[strcspn(buf, "\n")] = 0; 
                    printf("Received %d bytes from client %d: %s\n", len, client, buf);

                    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</ h1></ body></ html> \n";
                    if (send(client, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        exit(EXIT_FAILURE);
                    }
                }
                close(client);
            }
        }
    }
    getchar();
    killpg(0, SIGKILL);
    close(server);

    return 0;
}