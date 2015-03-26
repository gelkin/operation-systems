#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "filter.h"
#include "helpers.h"

#define BUF_SIZE 4096
#define DELIM '\n'

char* get_line(char *buf, size_t n)
{
    char* line = (char*) malloc(n + 1);
    memcpy(line, buf, n);
    line[n] = '\0';

    return line;
}

int process_buffer(char *buf, size_t size, int argc, char* argv[])
{
    int offset = 0;
    int counter = 0;
    while (offset < size)
    {
        while (buf[counter] != DELIM && counter < size)
        {
            ++counter;
        }

        int line_length = counter - offset;
        char* line = get_line(buf + offset, line_length);
        argv[argc - 1] = line;

        int res = spawn(argv[0], argv);
        if (res == EXIT_SUCCESS)
        {
            int on_write = write_(STDOUT_FILENO, line, line_length);
            if (on_write < 0)
            {
                perror("Error while writing");
                return -1;
            }
            
            // write '\n'
            on_write = write_(STDOUT_FILENO, "\n", 1);
            if (on_write < 0)
            {
                perror("Error while writing");
                return -1;
            }
        }
        else if (res < 0)
        {
            perror("Error while calling 'spawn'");
            return -1;
        }

        free(line);

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

// shift args to the left EXEPT NULL-argument at the end
void shift_left_args(int argc, char* argv[])
{
    for (int i = 0; i < (argc - 1); ++i)
    {
        argv[i] = argv[i + 1];
    }
}

int main(int argc, char* argv[])
{
    shift_left_args(argc, argv);

    char buf[BUF_SIZE];
    int read_bytes = 0;
    int offset = 0;
    while ((read_bytes = read_until(STDIN_FILENO, buf + offset, BUF_SIZE - offset, DELIM)) > 0)
    {
        read_bytes += offset;

        int size = last_delim_pos(buf, read_bytes) + 1;

        int on_write = process_buffer(buf, size, argc, argv);
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
        int on_write = process_buffer(buf, offset, argc, argv);
        if (on_write < 0)
        {
            perror("Error while writing");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

