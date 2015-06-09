#define _POSIX_SOURCE

#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>

#include <bufio.h>

#define MAX_FDS_NUMBER 256
#define MAX_BUF_PAIRS 127
#define BUF_SIZE 1024
#define FIRST_COND 1
#define SECOND_COND 2

struct buf_pair {
    struct buf_t* first_buf;
    struct buf_t* second_buf;
    int is_first_alive;
    int is_second_alive;
};

void free_all(struct buf_pair* buffers, size_t nbuffers)
{
    for (size_t i = 0; i < nbuffers; ++i)
    {
        buf_free(buffers[i].first_buf);
        buf_free(buffers[i].second_buf);
    }
}

int init_fds_and_buffers(struct pollfd* fds, struct buf_pair* buffers)
{
    for (size_t i = 0; i < MAX_FDS_NUMBER; ++i)
    {
        fds[i].fd = -2;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    for (size_t i = 0; i < MAX_BUF_PAIRS; ++i)
    {
        buffers[i].first_buf = buf_new(BUF_SIZE);
        buffers[i].second_buf = buf_new(BUF_SIZE);
        if (buffers[i].first_buf == NULL || buffers[i].second_buf == NULL)
        {
            free(buffers[i].first_buf);
            free(buffers[i].second_buf);

            free_all(buffers, i);
            return -1;
        }
        
        buffers[i].is_first_alive = 1;
        buffers[i].is_second_alive = 1;
    }
    return 0;
}

int get_socket(const char* port, const struct addrinfo* hints)
{
    struct addrinfo* result;

    if (getaddrinfo("localhost", port, hints, &result) != 0)
    {
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
            return -1;
        }
    }
     
    if (rp == NULL) // no address succeeded
    {
        return -1;
    }
    
    freeaddrinfo(result);

    if (listen(sock, 100) == -1)
    {
        return -1;
    }

    return sock;
}

int transfer_data(struct pollfd* first, struct pollfd* second, struct buf_pair* buf)
{
    if (((first->revents & POLLERR) != 0) || ((second->revents & POLLERR) != 0))
    {
        return -1;
    }
    
    // first -> second
    if ((second->revents & POLLOUT) != 0)
    {
        if (buf_flush(second->fd, buf->first_buf, 1) < 0)
        {
            return -1;
        }
    }
    if ((first->revents & POLLIN) != 0)
    {
        ssize_t read_bytes = 0;
        size_t buf_sz = buf_size(buf->first_buf);
        if ((read_bytes = buf_fill(first->fd, buf->first_buf, buf_sz + 1)) <= buf_sz)
        {
            if (read_bytes < 0)
            {
                return -1;
            }
            
            // we met EOF
            buf->is_first_alive = 0;
            shutdown(first->fd, SHUT_RD);  
        }
    }
    
    // second -> first
    if ((first->revents & POLLOUT) != 0)
    {
        if (buf_flush(first->fd, buf->second_buf, 1) < 0)
        {
            return -1;
        }
    }
    if ((second->revents & POLLIN) != 0)
    {
        ssize_t read_bytes = 0;
        size_t buf_sz = buf_size(buf->second_buf);
        if ((read_bytes = buf_fill(second->fd, buf->second_buf, buf_sz + 1)) <= buf_sz)
        {
            if (read_bytes < 0)
            {
                return -1;
            }
            // we met EOF
            buf->is_second_alive = 0;
            shutdown(second->fd, SHUT_RD);  
        }
    }

    first->revents = 0;
    second->revents = 0;

    return 0;
}

int set_events(struct pollfd* first, struct pollfd* second, struct buf_pair* buf)
{
    if ((buf->is_first_alive == 0) && (buf->is_second_alive == 0))
    {
        return -1;
    }

    first->events = 0;
    if ((buf->is_first_alive == 1) && buf_size(buf->first_buf) < BUF_SIZE)
    {
        first->events |= POLLIN;
    }
    if (buf_size(buf->second_buf) > 0)
    {
        first->events |= POLLOUT;
    }

    second->events = 0;
    if ((buf->is_second_alive == 1) && buf_size(buf->second_buf) < BUF_SIZE)
    {
        second->events |= POLLIN;
    }
    if (buf_size(buf->first_buf) > 0)
    {
        second->events |= POLLOUT;
    }

    return 0;
}

void copy_clients(struct pollfd* from, struct pollfd* to)
{
    to->fd = from->fd;
    to->events = from->events;
    to->revents = from->revents;
}

void swap_clients(struct pollfd* first, struct pollfd* second)
{
    struct pollfd tmp;
    copy_clients(first, &tmp);
    copy_clients(second, first);
    copy_clients(&tmp, second);
}

void copy_buffers(struct buf_pair* from, struct buf_pair* to)
{
    to->first_buf = from->first_buf;
    to->second_buf = from->second_buf;
    to->is_first_alive = from->is_first_alive;
    to->is_second_alive = from->is_second_alive;
}

void swap_buffers(struct buf_pair* first, struct buf_pair* second)
{
    struct buf_pair tmp;
    copy_buffers(first, &tmp);
    copy_buffers(second, first);
    copy_buffers(&tmp, second);
}

void remove_clients(int pos, struct pollfd* fds, struct buf_pair* buffers, size_t nfds)
{
    swap_clients(&fds[pos], &fds[nfds - 2]);
    swap_clients(&fds[pos + 1], &fds[nfds - 1]);
    swap_buffers(&buffers[(pos - 2) / 2], &buffers[(nfds - 2) / 2]);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        exit(EXIT_FAILURE);
    }
    
    struct pollfd fds[MAX_FDS_NUMBER];
    struct buf_pair buffers[MAX_BUF_PAIRS];
    if (init_fds_and_buffers(fds, buffers) < 0)
    {
        exit(EXIT_FAILURE);
    }
    nfds_t nfds = 0;

    // register sockets
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
        free_all(buffers, MAX_BUF_PAIRS);
        exit(EXIT_FAILURE);
    }
    int second_sock = get_socket(argv[2], &hints);
    if (second_sock == -1)
    {
        close(first_sock);
        free_all(buffers, MAX_BUF_PAIRS);
        exit(EXIT_FAILURE);
    }
    fds[0].fd = first_sock;
    fds[1].fd = second_sock;
    nfds += 2;

    int server_cond = FIRST_COND;
    while (1)
    {
        if (nfds == MAX_FDS_NUMBER)
        {
            fds[0].events = 0;
            fds[1].events = 0;
        }
        else if (server_cond == FIRST_COND)
        {
            fds[0].events = POLLIN;
            fds[1].events = 0;
        }
        else
        {
            fds[0].events = 0;
            fds[1].events = POLLIN;
        }

        if (poll(fds, nfds, -1) < 0 && errno != EINTR)
        {
            for (int i = 0; i < nfds; ++i)
            {
                close(fds[i].fd);
            }
            free_all(buffers, MAX_BUF_PAIRS);

            exit(EXIT_FAILURE);
        }
        
        for (int i = 2; i < nfds; i += 2)
        {
            if (transfer_data(&fds[i], &fds[i + 1], &buffers[(i - 2) / 2]) < 0)
            {
                // problem occured - need to remove clients 
                close(fds[i].fd);
                close(fds[i + 1].fd);
                remove_clients(i, fds, buffers, nfds);
                nfds -= 2;

                continue;
            }

            if (set_events(&fds[i], &fds[i + 1], &buffers[(i - 2) / 2]) < 0)
            {
                // problem occured - need to remove clients 
                close(fds[i].fd);
                close(fds[i + 1].fd);
                remove_clients(i, fds, buffers, nfds);
                nfds -= 2;
            }

        }

        if (nfds < MAX_FDS_NUMBER)
        {
            if (server_cond == SECOND_COND && ((fds[1].revents & POLLIN) != 0))
            {
                struct sockaddr_in client;
                socklen_t sz = sizeof(client);
                int first_accepted_fd = accept(fds[0].fd, 
                                       (struct sockaddr*)&client,
                                       &sz);
                int second_accepted_fd = accept(fds[1].fd,
                                        (struct sockaddr*)&client,
                                        &sz);

                if (first_accepted_fd == -1 || second_accepted_fd == -1)
                {
                    if (first_accepted_fd != -1)
                    {
                       close(first_accepted_fd);
                    }
                    if (second_accepted_fd != -1)
                    {
                       close(second_accepted_fd);
                    }
                }
                else
                {
                    fds[nfds].fd = first_accepted_fd; 
                    fds[nfds].events = POLLIN;
                    fds[nfds + 1].fd = second_accepted_fd;
                    fds[nfds + 1].events = POLLIN;
                    
                    size_t buf_pos = nfds / 2;
                    buf_set_size(buffers[buf_pos].first_buf, 0);
                    buf_set_size(buffers[buf_pos].second_buf, 0);
                    buffers[buf_pos].is_first_alive = 1;
                    buffers[buf_pos].is_second_alive = 1;
        
                    nfds += 2;
                }
                
                fds[0].revents = 0;
                fds[1].revents = 0;
                server_cond = FIRST_COND; 
            }

            if (server_cond == FIRST_COND && ((fds[0].revents & POLLIN) != 0))
            {
                server_cond = SECOND_COND;
            }
        }
    }

    exit(EXIT_SUCCESS);
}
