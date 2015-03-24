#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
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

int spawn(const char * file, char * const argv [])
{
    int pid = fork();
    if (pid == 0)
    {
       int on_exec = execvp(file, argv);
       if (on_exec < 0)
       {
           return EXIT_FAILURE;
       }
    }
    else if (pid > 0)
    {
        int status = 0;
        int on_wait = wait(&status);
        if (WEXITSTATUS(status) == EXIT_FAILURE ||
            on_wait != pid) {
            return EXIT_FAILURE;
        }
    }
    else
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}





















