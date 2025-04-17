#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    // Client Setup --------------------------------------------------------------------------->
    int client_fd = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("[Client] socket() failed.");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("[Client] Invalid address/ Address not supported.");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("[Client] connect() failed.");
        exit(EXIT_FAILURE);
    }

    // End Client Setup --------------------------------------------------------------------------->

    while (1)
    {
        // prompt the client to input a command
        printf("[Client] Enter message: ");
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';

        if (strcmp(buffer, "quit") == 0)
        {
            printf("[Client] Quitting...\n");
            send(client_fd, buffer, strlen(buffer), 0);
            break;
        }

        if (strstr(buffer, "download"))
        {
            // parse input and send the filename to the server
            strtok(buffer, " ");
            char *filename = strtok(NULL, " ");
            printf("%s\n", filename);
            send(client_fd, filename, strlen(filename), 0);

            // server will first respond with the file size
            int nbytes = read(client_fd, buffer, BUFFER_SIZE);

            if (nbytes <= 0)
            {
                perror("[Client] read() failed.");
                exit(EXIT_FAILURE);
            }

            // Note: I didn't implement any error handling for when the file don't exist
            printf("The file size is | %s | continue? (y/n)\n", buffer);
            long file_size = strtoul(buffer, NULL, 10);

            // Prompt for continuing
            printf("[Client] Enter message: ");
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);

            if (strstr(buffer, "y"))
            {
                // Tell the server to proceed
                send(client_fd, "y", 1, 0);

                printf("Where do you want to download the file to?\n");

                // Prompt for destination file name
                printf("[Client] Enter message: ");
                memset(buffer, 0, BUFFER_SIZE);
                fgets(buffer, BUFFER_SIZE, stdin);

                // Remove the newline preserved by fgets
                buffer[strlen(buffer) - 1] = '\0';

                printf("[Client] The destination filename is : %s\n", buffer);

                // buffer holds the destination file name
                // create space for the file
                void *out_buffer = malloc(file_size);
                FILE *out_file = fopen(buffer, "wb");

                int recv = 0;

                // Handle shortcount
                while (nbytes = read(client_fd, out_buffer, file_size))
                {
                    printf("[DEBUG] Writing %d bytes -> %s\n", nbytes, buffer);
                    fwrite(out_buffer, 1, nbytes, out_file);
                    recv += nbytes;

                    if (recv == file_size)
                        break;
                }

                printf("[DEBUG] %d bytes written -> %s\n", recv, buffer);

                free(out_buffer);
                fclose(out_file);
            }
        }

        if (strcmp(buffer, "quit") == 0)
        {
            printf("[Client] Server chatter quitting...\n");
            break;
        }
    }

    printf("[Client] Shutting down.\n");
    close(client_fd);
    return 0;
}
