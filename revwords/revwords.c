#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

#define BUF_SIZE 4096
#define DELIM ' '


void reverse(char *buf, size_t from, size_t to)
{
    int i = from;
    int j = to - 1;
    for (; i < j; ++i, --j)
    {
        char ch = buf[i];
        buf[i] = buf[j];
        buf[j] = ch;
    }
}

int write_revwords(char *buf, size_t size)
{
    int offset = 0;
    int counter = 0;
    while (offset < size)
    {
        // offset is still = 0 wtf?!!!
        while (buf[counter] != DELIM && counter < size)
        {
            ++counter;
        }

        reverse(buf, offset, counter);

        if (buf[counter] == DELIM)
        {
            offset = ++counter;
        }
        else
        {
            offset = counter;
        }
    }

    return write_(STDOUT_FILENO, buf, size);
}

int last_delim_pos(char *buf, size_t size)
{
    int pos = -1;
    for (int i = 0; i < size; ++i)
    {
        if (buf[i] == DELIM)
        {
            pos = i;
        }
    }

    return pos;
}

int main()
{
    char buf[BUF_SIZE];
    int read_bytes = 0;
    int offset = 0;
    while ((read_bytes = read_until(STDIN_FILENO, buf + offset, BUF_SIZE - offset, DELIM)) > 0)
    {
        read_bytes += offset;

        int size = last_delim_pos(buf, read_bytes) + 1;

        int on_write = write_revwords(buf, size);
        if (on_write < 0)
        {
            perror("Error while writing");
            return EXIT_FAILURE;
        }

        // put last word after DELIM to the begining of 'buf'
        memmove(buf, buf + size, read_bytes - size);
        offset = read_bytes - size;
    }

    if (read_bytes < 0)
    {
        perror("Error while reading");
        return EXIT_FAILURE;
    }
    else if (offset > 0)
    {
        int on_write = write_revwords(buf, offset);
        if (on_write < 0)
        {
            perror("Error while writing");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

