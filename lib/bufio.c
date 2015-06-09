#include "bufio.h"

struct buf_t
{
    size_t size;
    size_t capacity;
    char* buffer;
};

struct buf_t *buf_new(size_t capacity)
{
    struct buf_t* res = (struct buf_t*) malloc(sizeof(struct buf_t));
    if (res == NULL)
    {
        return NULL;
    }
    
    char* buffer = (char*) malloc(capacity * sizeof(char));
    if (buffer == NULL)
    {
        return NULL;
    }

    res->capacity = capacity;
    res->size = 0;
    res->buffer = buffer;

    return res;
}

void buf_free(struct buf_t * buffer)
{
    if (buffer->buffer != NULL)
    {
        free(buffer->buffer);
    }
    
    free(buffer);
}

size_t buf_capacity(struct buf_t * buffer)
{
#ifdef DEBUG
    if (buffer == NULL) 
    {
        abort();
    }
#endif

    return buffer->capacity;
}

size_t buf_size(struct buf_t * buffer)
{
#ifdef DEBUG
    if (buffer == NULL) 
    {
        abort();
    }
#endif

    return buffer->size;
}

ssize_t buf_fill(fd_t fd,struct buf_t *buf, size_t required)
{
#ifdef DEBUG
    if (buf == NULL) 
    {
        abort();
    }
#endif

    ssize_t read_bytes = 0;
    while ((buf->size < required) &&
           (read_bytes = read(fd, buf->buffer + buf->size, buf->capacity - buf->size)) > 0)
    {
       buf->size += read_bytes; 
    }

    if (read_bytes >= 0)
    {
        return buf->size;
    }
    else
    {
        // Only read-error may appear, so errno is already set as read-error.
        return -1;
    }
}

ssize_t buf_flush(fd_t fd, struct buf_t *buf, size_t required)
{
#ifdef DEBUG
    if (buf == NULL) 
    {
        abort();
    }
#endif

    ssize_t written_bytes = 0;
    size_t offset = 0;
    while ((offset < required) &&
           (written_bytes = write(fd, buf->buffer + offset, buf->size - offset)))
    {
        offset += written_bytes;
    }
    
    if (written_bytes >= 0)
    {
        memmove(buf->buffer, buf->buffer + offset, buf->size - offset);
        buf->size = buf->size - offset;
        return offset; // (old_size - new_size)
    }
    else
    {
        return -1;
    }
}

#define DELIM '\n'
#define END_OF_STR '\0'

ssize_t delim_pos(struct buf_t *buf, char delim)
{
    ssize_t len = -1;
    for (int i = 0; i < buf_size(buf); ++i)
    {
        if (buf->buffer[i] == delim)
        {
            len = i;
            break;
        }
    }
    return len;
}

// return (size of line + 1) - '\0' symbol
ssize_t buf_getline(fd_t fd, struct buf_t *buf, char* dest)
{
    ssize_t len = 0;
    ssize_t read_bytes = 0; 
    ssize_t offset = 0; 

    do
    {
        len = delim_pos(buf, DELIM);

        if (len != -1)
        {
            memcpy(dest, buf->buffer, len);
            dest[len] = END_OF_STR;
        
            // move buf
            memmove(buf->buffer, buf->buffer + len + 1, buf_size(buf) - len - 1); 
            buf->size = buf_size(buf) - len - 1;

            return len + 1; // with '\0' symbol
        }
        else
        {
            memcpy(dest + offset, buf->buffer, buf_size(buf));
            
            // make buffer empty
            offset += buf->size;
            buf->size = 0;
        }
    } while ((read_bytes = buf_fill(fd, buf, buf_capacity(buf))) > 0);

    if (read_bytes == 0)
    {
        if (offset == 0)
        {
            return 0; // EOF
        }
        else
        {
            // next read will meet EOF 
            dest[offset] = END_OF_STR;
            return offset + 1; // with '\0' symbol
        }
    }
    else
    {
        perror("Error while filling buffer");
        return -1;
    }
}

// return buf_size after flushing
ssize_t buf_write(fd_t fd, struct buf_t *buf, char* src, size_t len)
{
    ssize_t to_write = len;
    ssize_t offset = 0;
    while (to_write > 0)
    {
        if (buf_capacity(buf) - buf_size(buf) >= to_write)
        {
            // write to buffer and that's it
            memcpy(buf->buffer + buf_size(buf), src + offset, to_write);
            buf->size += to_write;
            
            break;
        }
        else
        {
            ssize_t cur_to_write = buf_capacity(buf) - buf_size(buf); 
            to_write -= cur_to_write; 

            memcpy(buf->buffer + buf_size(buf), src + offset, cur_to_write);
            buf->size += cur_to_write;
            offset += cur_to_write;

            ssize_t on_flush = buf_flush(fd, buf, buf_size(buf));
            if (on_flush < 0)
            {
                perror("Error while flushing");
                return -1;
            }
        }
    }
    
    return buf_size(buf);
}

void buf_set_size(struct buf_t* buf, size_t new_size)
{
    buf->size = new_size;
}
































