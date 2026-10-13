
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 2048

// Replace with your actual roll number
#define ROLL "24105003"

int get_port(void) {
    return 30000 + atoi(ROLL + strlen(ROLL) - 3);
}

int get_type(void) {
    int len = strlen(ROLL);
    int sum = 0;

    for (int i = len - 3; i < len; i++)
        sum += ROLL[i] - '0';

    return sum % 3;
}

void process(char *msg, char *out) {
    int type = get_type();

    if (type == 0) {
        // Type A: Text Transformer
        char *cmd = strtok(msg, "|");
        char *text = strtok(NULL, "");

        if (!cmd || !text) {
            strcpy(out, "Invalid request");
            return;
        }

        if (strcmp(cmd, "UP") == 0) {
            for (int i = 0; text[i]; i++)
                text[i] = toupper((unsigned char)text[i]);
        } else if (strcmp(cmd, "LOW") == 0) {
            for (int i = 0; text[i]; i++)
                text[i] = tolower((unsigned char)text[i]);
        } else if (strcmp(cmd, "REV") == 0) {
            int n = strlen(text);
            for (int i = 0; i < n / 2; i++) {
                char temp = text[i];
                text[i] = text[n - 1 - i];
                text[n - 1 - i] = temp;
            }
        } else {
            strcpy(out, "Invalid command");
            return;
        }

        strcpy(out, text);
    }
    else if (type == 1) {
        // Type B: Integer List Processor
        char *op = strtok(msg, "|");
        char *ns = strtok(NULL, "|");
        char *nums = strtok(NULL, "|");

        if (!op || !ns || !nums) {
            strcpy(out, "Invalid request");
            return;
        }

        int n = atoi(ns);
        if (n < 1 || n > 500) {
            strcpy(out, "Invalid number of integers");
            return;
        }

        long long sum = 0, x, max = 0, min = 0;
        char *p = nums;
        int count = 0;

        while (count < n) {
            while (*p && isspace((unsigned char)*p))
                p++;

            if (!*p)
                break;

            char *end;
            x = strtoll(p, &end, 10);

            if (end == p) {
                strcpy(out, "Invalid integer list");
                return;
            }

            if (count == 0 || x > max) max = x;
            if (count == 0 || x < min) min = x;

            sum += x;
            count++;
            p = end;
        }

        if (count != n) {
            strcpy(out, "Incorrect number of integers");
            return;
        }

        if (strcmp(op, "SUM") == 0)
            snprintf(out, BUFFER_SIZE, "%lld", sum);
        else if (strcmp(op, "MAX") == 0)
            snprintf(out, BUFFER_SIZE, "%lld", max);
        else if (strcmp(op, "MIN") == 0)
            snprintf(out, BUFFER_SIZE, "%lld", min);
        else if (strcmp(op, "AVG") == 0)
            snprintf(out, BUFFER_SIZE, "%.2f",
                     (double)sum / n);
        else
            strcpy(out, "Invalid operation");
    }
    else {
        // Type C: String Statistics
        int len = strlen(msg);
        int vowels = 0, consonants = 0, digits = 0;

        for (int i = 0; msg[i]; i++) {
            unsigned char c = (unsigned char)msg[i];

            if (isalpha(c)) {
                char ch = tolower(c);

                if (ch == 'a' || ch == 'e' || ch == 'i' ||
                    ch == 'o' || ch == 'u')
                    vowels++;
                else
                    consonants++;
            }

            if (isdigit(c))
                digits++;
        }

        snprintf(out, BUFFER_SIZE,
                 "LEN=%d;VOWELS=%d;CONSONANTS=%d;DIGITS=%d",
                 len, vowels, consonants, digits);
    }
}

int main(void) {
    int sock;
    struct sockaddr_in server, client;
    socklen_t client_len;
    char buffer[BUFFER_SIZE], response[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(get_port());

    if (bind(sock, (struct sockaddr *)&server,
             sizeof(server)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    printf("Server running on port %d\n", get_port());
    printf("Service Type: %c\n", 'A' + get_type());

    while (1) {
        client_len = sizeof(client);

        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client, &client_len);

        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        buffer[n] = '\0';

        printf("\nClient: %s:%d\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port));

        char *sep = strchr(buffer, '#');

        if (!sep) {
            strcpy(response, "ERROR: Unauthorized client");
        } else {
            *sep = '\0';

            if (strcmp(buffer, ROLL) != 0) {
                strcpy(response, "ERROR: Unauthorized client");
            } else {
                char *msg = sep + 1;

                printf("Received: %s\n", msg);

                if (strcmp(msg, "EXIT") == 0) {
                    strcpy(response, "Server shutting down");

                    sendto(sock, response, strlen(response), 0,
                           (struct sockaddr *)&client, client_len);

                    printf("Sent: %s\n", response);
                    printf("Server exiting.\n");
                    break;
                }

                process(msg, response);
            }
        }

        sendto(sock, response, strlen(response), 0,
               (struct sockaddr *)&client, client_len);

        printf("Sent: %s\n", response);
    }

    close(sock);
    return 0;
}
