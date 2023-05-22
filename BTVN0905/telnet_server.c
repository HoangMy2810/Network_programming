#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MAX_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char username[MAX_LEN];
    char password[MAX_LEN];
} client_t;

int check_user(char *username, char *password)
{
    FILE *fp = fopen("idname.txt", "r");
    if (fp == NULL)
    {
        return -1;
    }
    char buf[MAX_LEN];
    while (fgets(buf, MAX_LEN, fp) != NULL)
    {
        buf[strcspn(buf, "\n")] = 0;

        char user[MAX_LEN];
        char pass[MAX_LEN];
        sscanf(buf, "%s %s", user, pass);

        if (strcmp(username, user) == 0 && strcmp(password, pass) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void do_someth(int sockfd, char *command)
{
    char cmd[MAX_LEN];
    sprintf(cmd, "%s > out.txt", command);

    int status = system(cmd);
    if (status == 0)
    {
        FILE *fp = fopen("out.txt", "r");
        if (fp == NULL)
        {
            return;
        }
        char buf[MAX_LEN];
        while (fgets(buf, MAX_LEN, fp) != NULL)
        {
            if (send(sockfd, buf, strlen(buf), 0) < 0)
            {
                perror("send() failed");
                continue;
            }
        }
        char *msg = "\nNhap lenh ban muon thuc hien: ";
        if (send(sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            return;
        }
        fclose(fp);
    }
    else
    {
        char *msg = "Khong tim thay lenh! Nhap lai: ";
        if (send(sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    client_t clients[MAX_CLIENTS];
    int num_users = 0;
    fd_set readfds;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        if (num_users == 0)
        {
            printf("Doi user ket noi....");
        }
        else
        {
            for (int i = 0; i < num_users; i++)
            {
                FD_SET(clients[i].sockfd, &readfds);
            }
        }

        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select() failed");
            continue;
        }

        if (FD_ISSET(sockfd, &readfds))
        {
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t client_addr_len = sizeof(client_addr);
            int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_sockfd < 0)
            {
                perror("accept() failed");
                continue;
            }

            if (num_users < MAX_CLIENTS)
            {
                clients[num_users].sockfd = client_sockfd;
                clients[num_users].addr = client_addr;
                strcpy(clients[num_users].username, "");
                strcpy(clients[num_users].password, "");
                num_users++;
                printf("New client connected.\n");

                char *msg = "Nhap \"username password\": ";
                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
            else
            {
                char *msg = "Too many connections.\n";
                send(client_sockfd, msg, strlen(msg), 0);
                close(client_sockfd);
            }
        }

        for (int i = 0; i < num_users; i++)
        {
            if (FD_ISSET(clients[i].sockfd, &readfds))
            {
                char msg[MAX_LEN];
                memset(msg, 0, MAX_LEN);
                int msg_len = recv(clients[i].sockfd, msg, MAX_LEN, 0);
                if (msg_len < 0)
                {
                    perror("recv() failed");
                    continue;
                }
                else if (msg_len == 0)
                {
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(clients[i].addr.sin_addr),
                           ntohs(clients[i].addr.sin_port));
                    close(clients[i].sockfd);

                    clients[i] = clients[num_users - 1];
                    num_users--;
                    FD_CLR(clients[i].sockfd, &readfds);
                    continue;
                }
                else
                {
                    msg[strcspn(msg, "\n")] = 0;

                    if (strcmp(clients[i].username, "") == 0 && strcmp(clients[i].password, "") == 0)
                    {
                        char username[MAX_LEN];
                        char password[MAX_LEN];
                        char temp[MAX_LEN];
                        int ret = sscanf(msg, "%s %s %s", username, password, temp);
                        if (ret == 2)
                        {
                            if (check_user(username, password) == 1)
                            {
                                strcpy(clients[i].username, username);
                                strcpy(clients[i].password, password);

                                char *msg = "Dung cu phap. Hay gui lenh: ";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else
                            {
                                char *msg = "Nhap sai. Yeu cau nhap lai.\n ";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            char *msg = "Nhap sai. Yeu cau nhap lai.\n";
                            send(clients[i].sockfd, msg, strlen(msg), 0);
                        }
                    }
                    else
                    {
                        if (strcmp(msg, "quit") == 0 || strcmp(msg, "exit") == 0)
                        {
                            close(clients[i].sockfd);
                            clients[i] = clients[num_users - 1];
                            num_users--;
                            FD_CLR(clients[i].sockfd, &readfds);
                        }
                        else
                        {
                            do_someth(clients[i].sockfd, msg);
                        }
                    }
                }
            }
        }
    }
    close(sockfd);

    return 0;
}
