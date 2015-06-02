// TODO
#define _POSIX_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "helpers.h"


ssize_t read_(int fd, void *buf, size_t count)
{
    ssize_t read_bytes = 0;
    ssize_t offset = 0;
    while ((read_bytes = read(fd, buf + offset, count - offset)) > 0)
    {
       offset += read_bytes;
    }

    if (read_bytes == 0)
    {
        return offset;
    }
    else
    {
        return read_bytes;
    }
}

ssize_t write_(int fd, const void *buf, size_t count)
{
    ssize_t written_bytes = 0;
    ssize_t offset = 0;
    while ((written_bytes = write(fd, buf + offset, count - offset)) > 0)
    {
       offset += written_bytes;
    }

    if (written_bytes == 0)
    {
        return offset;
    }
    else
    {
        return written_bytes;
    }
}

ssize_t read_until(int fd, void *buf, size_t count, char delimiter)
{
    ssize_t read_bytes = 0;
    ssize_t offset = 0;
    int got_delim = 0;
    // when it gets DELIM, it doesn't read, because of '&&' laziness
    while (!got_delim && (read_bytes = read(fd, buf + offset, count - offset)) > 0)
    {
        for (int i = offset; (i < offset + read_bytes) && !got_delim; ++i)
        {
            if (((char *) buf)[i] == delimiter)
            {
                got_delim = 1;
            }
        }

        offset += read_bytes;
    }

    if (read_bytes >= 0)
    {
        return offset;
    }
    else
    {
        return read_bytes; // -1
    }
}

// redirect stdout and stderr to /dev/null
static int redirect()
{
    int fd = open("/dev/null", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }
    int on = dup2(fd, STDOUT_FILENO);
    if (on < 0)
    {
        return -1;
    }
    on = dup2(fd, STDERR_FILENO);
    if (on < 0)
    {
        return -1;
    }

    return 1;
}

int spawn(const char * file, char * const argv [])
{
    int status = 0;
    int pid = fork();
    if (pid == 0)
    {
        int on_redirect = redirect();
        if (on_redirect < 0)
        {
            perror("Couldn't redirect");
            return -1;
        }

        int on_exec = execvp(file, argv);
        if (on_exec < 0)
        {
            return EXIT_FAILURE;
        }
    }
    else if (pid > 0)
    {
        int on_wait = wait(&status);
        if (WEXITSTATUS(status) == EXIT_FAILURE ||
            on_wait != pid) {
            return EXIT_FAILURE;
        }
    }
    else
    {
        perror("Couldn't make fork");
        return -1;
    }

    return status;
}

/*
struct execargs_t
{
    char* file;
    char** argv;
    int reader_fd;
    int writer_fd;
};

int exec(struct execargs_t* args)
{
    int pid = fork();
    if (pid == 0)
    {
        if (dup2(args->reader_fd, STDIN_FILENO) < 0)
        {
            perror("dup2");
            return -1;
        }
        if (close(args->reader_fd) < 0)
        {
            perror("close");
            return -1; 
        }

        if (dup2(args->writer_fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            return -1;
        }
        if (close(args->writer_fd) < 0)
        {
            perror("close");
            return -1;
        }

        int on_exec = execvp(args->file, args->argv);
        if (on_exec < 0)
        {
            return -1;
        }
    }
    else if (pid < 0)
    {
        perror("fork");
        return -1;
    }
    return pid;
}

// TODO FUUU it's disgusting
int* pids;
size_t pids_number;

void pipe_sigint_handler(int signal)
{
    for (int i = 0; i < pids_number; ++i)
    {
        if (pids[i] != 0)
        {
            if (kill(pids[i], SIGTERM) < 0)
            {
                perror("termination");
                exit(EXIT_FAILURE);
            }
        }
    }
    exit(EXIT_SUCCESS);
}

int terminate_child_processes(int* pids, size_t size)
{
    for (int i = 0; i < size; ++i)
    {
        if (kill(pids[i], SIGTERM) < 0)
        {
            perror("termination");
            return -1;
        }
    }
    return 0;
}

int runpiped(struct execargs_t** programs, size_t n)
{
    pids = (int*) malloc(sizeof(int) * n);
    pids_number = n;
    for (int i = 0; i < n; ++i)
    {
        pids[i] = 0;
    }
    
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = pipe_sigint_handler;
    if (sigaction(SIGINT, &act, 0) < 0)
    {
        perror("sigaction");
        return -1;
    }

    int read_fd = STDIN_FILENO;
    for (int i = 0; i < n; ++i)
    {
        if (i < n - 1)
        {
            int pipefd[2];
            pipe(pipefd);

            programs[i]->reader_fd = read_fd;
            programs[i]->writer_fd = pipefd[1];
            int on_exec = exec(programs[i]);
            if (on_exec < 0)
            {
                terminate_child_processes(pids, i);
                return -1;
            }
            pids[i] = on_exec;
            read_fd = pipefd[0];
        }
        else
        {
            programs[i]->reader_fd = read_fd;
            programs[i]->writer_fd = STDOUT_FILENO;
            int on_exec = exec(programs[i]);
            if (on_exec < 0)
            {
                terminate_child_processes(pids, i);
                return -1;
            }
            pids[n - 1] = on_exec;
        }
    }

    int status;
    if (wait(&status) < 0) 
    {
        return -1;
    }

    if (terminate_child_processes(pids, n))
    {
        return -1;
    }

    return 0;
}

*/






















