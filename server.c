#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

long file_size = 0;

char *load_file(const char *filename)
{
    char *buffer = 0;
    long length;
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        // ERROR("Could not open file %s", filename);
        return NULL;
    }
    (void)fseek(file, 0, SEEK_END);
    length = ftell(file);
    (void)fseek(file, 0, SEEK_SET);
    buffer = (char *)malloc(length + 1);

    if (buffer != NULL)
    {
        (void)fread(buffer, 1, length, file);
        (void)fclose(file);
    }

    file_size = length;

    return buffer;
}

int main()
{
    // Server Setup --------------------------------------------------------------------------->
    int listen_fd, conn_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("[Server] bind() failed.");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd, 3) < 0)
    {
        perror("[Server] listen() failed.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Running on port %d\n", PORT);

    // Accept incoming connection
    if ((conn_fd = accept(listen_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("[Server] accept() failed.");
        exit(EXIT_FAILURE);
    }

    // End Server Setup --------------------------------------------------------------------------->

    // Receive and process commands
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int nbytes = read(conn_fd, buffer, BUFFER_SIZE);
        if (nbytes <= 0)
        {
            perror("[Server] read() failed.");
            exit(EXIT_FAILURE);
        }

        // current buffer holds filename sent by the client
        // if filename is quit, then we close down both server and client -> this is done when server receives a quit message and sents it right back to the client
        // client <- quit -> server

        printf("[Server] Received from client: %s\n", buffer);

        if (strcmp(buffer, "quit") == 0)
        {
            printf("[Server] Client quitting...\n");
            printf("[Server] Quitting...\n");
            send(conn_fd, buffer, strlen(buffer), 0);
            break;
        }

        FILE *fp = fopen(buffer, "rb");

        if (fp != NULL)
        {
            // load buffer with file content
            int sent = 0;
            char *file_content = load_file(buffer);

            printf("[Server] Loaded %s with size %ld into memory\n", buffer, file_size);

            // the first packet sent tells the client how big the file is

            char str[1024] = {0};
            sprintf(str, "%ld", file_size);
            send(conn_fd, str, BUFFER_SIZE, 0);

            printf("[Server] Sending client file size\n");

            // read whether not the client wants to continue

            printf("[Server] Waiting for client to respond...\n");
            nbytes = read(conn_fd, buffer, 1);
            printf("[Server] Client response: %c\n", buffer[0]);

            if (buffer[0] != 'y')
            {
                printf("[Server] Client quitting...\n");
                printf("[Server] Quitting...\n");
                send(conn_fd, buffer, strlen(buffer), 0);
                break;
            }
            else
            {
                send(conn_fd, file_content, file_size, 0);
                printf("[Server] Sent %ld\n", file_size);
            }
        }
        else
        {
            memcpy(buffer, "Error 404 File Not Found", 25);
            send(conn_fd, buffer, strlen(buffer), 0);
        }
    }
    printf("[Server] Shutting down.\n");

    close(conn_fd);
    close(listen_fd);
    return EXIT_SUCCESS;
}
