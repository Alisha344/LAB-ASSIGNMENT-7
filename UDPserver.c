
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 2048

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server;
    socklen_t server_len = sizeof(server);
    char input[BUFFER_SIZE], message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    if (argc != 3) {
        printf("Usage: %s <server_ip> <roll>\n", argv[0]);
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    int port = 30000 +
               atoi(argv[2] + strlen(argv[2]) - 3);
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1],
                  &server.sin_addr) != 1) {
        printf("Invalid server IP address\n");
        close(sock);
        return 1;
    }

    printf("Connected to UDP server at %s:%d\n",
           argv[1], port);

    while (1) {
        printf("Enter request: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            strcpy(input, "EXIT");
        } else {
            input[strcspn(input, "\n")] = '\0';
        }

        snprintf(message, sizeof(message), "%s#%s",
                 argv[2], input);

        if (sendto(sock, message, strlen(message), 0,
                   (struct sockaddr *)&server,
                   server_len) < 0) {
            perror("sendto");
            break;
        }

        int n = recvfrom(sock, response,
                         sizeof(response) - 1, 0,
                         (struct sockaddr *)&server,
                         &server_len);

        if (n < 0) {
            perror("recvfrom");
            break;
        }

        response[n] = '\0';
        printf("Server response: %s\n", response);

        if (strcmp(input, "EXIT") == 0)
            break;
    }

    close(sock);
    return 0;
}
