#include "bufio.h"

#define BUF_SIZE 4096

int main() 
{
    struct buf_t* buffer = buf_new(BUF_SIZE);
    ssize_t read_bytes = 0;
    while ((read_bytes == 0) && 
           (read_bytes = buf_fill(STDIN_FILENO, buffer, buf_capacity(buffer))) > 0)
    {
        int on_flush = buf_flush(STDOUT_FILENO, buffer, buf_size(buffer));
        if (on_flush == -1)
        {
            return EXIT_FAILURE;
        }

        // if read_bytes < bufcapacity -> we got EOF, so don't read anymore
        read_bytes = buf_capacity(buffer) - read_bytes;
    }

    if (read_bytes < 0)
    {
        return EXIT_FAILURE;
    }

    buf_free(buffer);
    return EXIT_SUCCESS;
}
