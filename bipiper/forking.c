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

int get_socket(const char* port, const struct addrinfo* hints)
{
    struct addrinfo* result;

    if (getaddrinfo("localhost", port, hints, &result) != 0)
    {
        perror("getaddrinfo");
        return -1;
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
            return -1;
        }
        if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            break; // success
        }
        if (close(sock) == -1)
        {
            perror("close");
            return -1;
        }
    }
     
    if (rp == NULL) // no address succeeded
    {
        perror("Couldn't bind");
        return -1;
    }
    
    freeaddrinfo(result);

    if (listen(sock, 1) == -1)
    {
        perror("listen");
        return -1;
    }

    return sock;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        perror("few arguments");
        exit(EXIT_FAILURE);
    }
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int first_sock = get_socket(argv[1], &hints);
    if (first_sock == -1)
    {
        exit(EXIT_FAILURE);
    }
    int second_sock = get_socket(argv[2], &hints);
    if (second_sock == -1)
    {
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // first client
        struct sockaddr_in first_client;
        socklen_t first_sz = sizeof(first_client);
        int first_fd = accept(first_sock, (struct sockaddr*)&first_client, &first_sz);
        if (first_fd == -1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // second client
        struct sockaddr_in second_client;
        socklen_t second_sz = sizeof(second_client);
        int second_fd = accept(second_sock, (struct sockaddr*)&second_client, &second_sz);
        if (second_fd == -1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // first fork()
        int first_pid = fork();
        if (first_pid == 0)
        {
            struct buf_t* buf = buf_new(BUF_SIZE);
            if (buf == NULL)
            {
                perror("buf_new error");
                exit(EXIT_FAILURE);
            }

            ssize_t read_bytes = 0;
            while ((read_bytes = buf_fill(first_fd, buf, 1)) > 0)
            {
                if (buf_flush(second_fd, buf, 1) == -1)
                {
                    perror("buf_flush");
                    exit(EXIT_FAILURE);
                }
            }

            if (read_bytes < 0)
            {
                perror("buf_fill");
                exit(EXIT_FAILURE);
            }

            if (buf_flush(second_fd, buf, buf_size(buf)) < 0)
            {
                perror("buf_flush");
                exit(EXIT_FAILURE);
            }

            buf_free(buf);

            if (close(first_fd) == -1 || close(second_fd) == -1) 
            {
                perror("close");
                exit(EXIT_FAILURE); 
            }
            exit(EXIT_SUCCESS);
        }
        else if (first_pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
 
        // second fork()
        int second_pid = fork();
        if (second_pid == 0)
        {
            struct buf_t* buf = buf_new(BUF_SIZE);
            if (buf == NULL)
            {
                perror("buf_new error");
                exit(EXIT_FAILURE);
            }

            ssize_t read_bytes = 0;
            while ((read_bytes = buf_fill(second_fd, buf, 1)) > 0)
            {
                if (buf_flush(first_fd, buf, 1) == -1)
                {
                    perror("buf_flush");
                    exit(EXIT_FAILURE);
                }
            }

            if (read_bytes < 0)
            {
                perror("buf_fill");
                exit(EXIT_FAILURE);
            }

            if (buf_flush(first_fd, buf, buf_size(buf)) < 0)
            {
                perror("buf_flush");
                exit(EXIT_FAILURE);
            }

            buf_free(buf);

            if (close(first_fd) == -1 || close(second_fd) == -1) 
            {
                perror("close");
                exit(EXIT_FAILURE); 
            }
            exit(EXIT_SUCCESS);
        }
        else if (second_pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }       

        
        if (close(first_fd) == -1 || close(second_fd) == -1) 
        {
            perror("close");
            exit(EXIT_FAILURE); 
        }
    }

    exit(EXIT_SUCCESS);
}
