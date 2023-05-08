#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char id[20];
    char name[50];
} client ; 

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == -1)
    {
        perror("scoket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if(bind(server, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    if(listen(server, 5) == -1)
    {
        perror("listen() failed");
        return 1;
    }

    struct client clients[5];
    int numClients = 0;
    while (1)
    {
        fd_set fdread;
        FD_ZERO(&fdread);
        FD_SET(server, &fdread);

        if (numClients)
        {
            for (int i = 0; i < numClients; i++)
            {
                FD_SET(clients[i].sockfd, &fdread);
            }
        }

        if (select(FD_SETSIZE, &fdread, NULL, NULL, NULL) < 0)
        {
            perror("select() failed");
            continue;
        }

        if (FD_ISSET(server, &fdread))
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client < 0)
            {
                perror("accept() failed");
                continue;
            }

            if (numClients < 5)
            {
                clients[numClients].sockfd = client;
                clients[numClients].addr = client_addr;
                strcpy(clients[numClients].id, "");
                strcpy(clients[numClients].name, "");
                numClients++;
                printf("New client connected");
                char *buf = "Enter your ID and Name: ";
                if (send(client, buf, strlen(buf), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
        }

        for (int i = 0; i < numClients; i++)
        {
            if (FD_ISSET(clients[i].sockfd, &fdread))
            {
                char buf[1024];
                memset(buf, 0, strlen(buf));
                int len = recv(clients[i].sockfd, buf, 1024, 0);
                if (len < 0)
                {
                    perror("recv() failed");
                    continue;
                }
                else if (len == 0)
                {
                    printf("1 client disconnected");
                    for (int j = i; j < numClients - 1; j++)
                    {
                        clients[j] = clients[j + 1];
                    }
                    numClients--;
                    FD_CLR(clients[i].sockfd, &fdread);

                    continue;
                }
                else
                {
                    buf[len] = '\0';
                    if (strcmp(clients[i].id, "") == 0 && strcmp(clients[i].name, "") == 0)
                    {
                        char id[20], name[50];
                        int ret = sscanf(buf, "%[^:]: %s", id, name);
                        if (ret == 2)
                        {
                            strcpy(clients[i].id, id);
                            strcpy(clients[i].name, name);
                            printf("Client registered");
                            if (send(clients[i].sockfd, "Welcome to chat_server!\n", 35, 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                        else
                        {
                            char *question = "Enter your \"client_id: client_name\": ";
                            if (send(clients[i].sockfd, question, strlen(question), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            continue;
                        }
                    }
                    else
                    {
                        char message[256+50];
                        memset(message, 0, strlen(message) + 50);
                        sprintf(message, "%s: %s", clients[i].id, buf);
                        for (int j = 0; j < numClients; j++)
                        {
                            if (j != i)
                            {
                                if (send(clients[j].sockfd, message, strlen(message), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(server);
}