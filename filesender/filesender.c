#define _POSIX_SOURCE

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "bufio.h"

#define BUF_SIZE 4096

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        perror("few arguments");
        exit(EXIT_FAILURE);
    }
    
    struct addrinfo hints;
    struct addrinfo* result;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if (getaddrinfo("localhost", argv[1], &hints, &result) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int sock;
    struct addrinfo* rp;
    int one = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1)
        {
            continue;
        }
        // must be set on each socket prior to calling bind
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1)
        {
            exit(EXIT_FAILURE);
        }
        if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break; // success
        }
        close(sock);
    }
     
    if (rp == NULL) // no address succeeded
    {
        perror("Couldn't bind");
        exit(EXIT_FAILURE);
    }
    
    freeaddrinfo(result);

    if (listen(sock, 1) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    while (1)
    {
        struct sockaddr_in client;
        socklen_t sz = sizeof(client);
        int fd = accept(sock, (struct sockaddr*)&client, &sz);
        if (fd == -1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        int pid = fork();
        if (pid == 0)
        {
            int in = open(argv[2], O_RDONLY);
            if (in == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            struct buf_t* buf = buf_new(BUF_SIZE);
            if (buf == NULL)
            {
                perror("buf_new error");
                exit(EXIT_FAILURE);
            }

            ssize_t read_bytes = 0;
            while ((read_bytes = buf_fill(in, buf, buf_capacity(buf))) > 0)
            {
                if (buf_flush(fd, buf, buf_size(buf)) == -1)
                {
                    perror("buf_flush");
                    exit(EXIT_FAILURE);
                }

                // if read_bytes < bufcapacity -> we got EOF, so don't read anymore
                if (read_bytes < buf_capacity(buf))
                {
                    break;
                }
            }

            if (read_bytes < 0)
            {
                perror("buf_fill");
                exit(EXIT_FAILURE);
            }

            buf_free(buf);
            if (close(in) == -1 || close(fd) == -1) 
            {
                perror("close");
                exit(EXIT_FAILURE); 
            }
            exit(EXIT_SUCCESS);
        }
        else if (pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (close(fd) == -1)
        {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
