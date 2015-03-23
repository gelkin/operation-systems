#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

#define BUF_SIZE 4096
#define LEN_BUF_SIZE 20
#define DELIM ' '

int write_lenwords(char *buf, size_t size)
{
    char buf_with_len[LEN_BUF_SIZE];

    int offset = 0;
    int counter = 0;
    while (offset < size)
    {
        // offset is still = 0 wtf?!!!
        while (buf[counter] != DELIM && counter < size)
        {
            ++counter;
        }

        int len = counter - offset;
        int chars_to_print = sprintf(buf_with_len, "%d ", len);
        if (chars_to_print < 0)
        {
            return -1;
        }
        int on_write = write_(STDOUT_FILENO, buf_with_len, chars_to_print);
        if (on_write < 0)
        {
            return -1;
        }

        if (buf[counter] == DELIM)
        {
            offset = ++counter;
        }
        else
        {
            offset = counter;
        }
    }

    return 1;
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

        int on_write = write_lenwords(buf, size);
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
        int on_write = write_lenwords(buf, offset);
        if (on_write < 0)
        {
            perror("Error while writing");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

