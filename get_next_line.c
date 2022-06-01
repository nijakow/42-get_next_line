#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE   42
#define MAX_FD        1024

/*
 *    G N L _ S t r i n g B u i l d e r
 */

struct GNL_StringBuilder
{
    unsigned int  alloc;
    unsigned int  fill;
    char*         chars;
};

void GNL_StringBuilder_Create(struct GNL_StringBuilder* builder)
{
    builder->alloc = 128;
    builder->fill  = 0;
    builder->chars = malloc(sizeof(char) * 128);
}

void GNL_StringBuilder_Destroy(struct GNL_StringBuilder* builder)
{
    if (builder->chars != NULL)
        free(builder->chars);
}

bool GNL_StringBuilder_HasSpace(struct GNL_StringBuilder* builder)
{
    return builder->fill < builder->alloc;
}

bool GNL_StringBuilder_ResizeTo(struct GNL_StringBuilder* builder, unsigned int new_size)
{
    char*         new_chars;
    unsigned int  index;

    new_chars = malloc(sizeof(char) * new_size);

    if (new_chars != NULL)
    {
        index = 0;
        while (index < builder->fill)
        {
            new_chars[index] = builder->chars[index];
            index++;
        }
        free(builder->chars);
        builder->alloc = new_size;
        builder->chars = new_chars;
    }

    return (new_chars != NULL);
}

bool GNL_StringBuilder_Finalize(struct GNL_StringBuilder* builder, char** into)
{
    char*         result;
    unsigned int  index;

    if (builder->fill == 0)
        *into = NULL;
    else {
        result = malloc(sizeof(char) * (builder->fill + 1));
        if (result == NULL)
            return false;
        index = 0;
        while (index < builder->fill)
        {
            result[index] = builder->chars[index];
            index++;
        }
        result[index] = '\0';
        *into = result;
    }
    return true;
}

bool GNL_StringBuilder_AppendChar(struct GNL_StringBuilder* builder, char c)
{
    if (!GNL_StringBuilder_HasSpace(builder))
        if (!GNL_StringBuilder_ResizeTo(builder, builder->alloc * 2))
            return false;
    builder->chars[builder->fill++] = c;
    return true;
}

/*
 *    G N L _ B u f f e r
 */

struct GNL_Buffer
{
    bool          initialized;
    int           fd;
    unsigned int  read_head;
    unsigned int  write_head;
    char          chars[BUFFER_SIZE];
};

void GNL_Buffer_Create(struct GNL_Buffer* buffer, int fd)
{
    buffer->initialized = true;
    buffer->fd          = fd;
    buffer->read_head   = 0;
    buffer->write_head  = 0;
}

bool GNL_Buffer_HasNext(struct GNL_Buffer* buffer)
{
    return buffer->read_head < buffer->write_head;
}

bool GNL_Buffer_Refresh(struct GNL_Buffer* buffer)
{
    ssize_t  bytes_read;

    bytes_read = read(buffer->fd, buffer->chars, sizeof(char) * BUFFER_SIZE);
    if (bytes_read <= 0)
        return false;
    buffer->write_head = (unsigned int) bytes_read;
    buffer->read_head  = 0;
    return true;
}

bool GNL_Buffer_ReadNext(struct GNL_Buffer* buffer, char* into)
{
    if (!GNL_Buffer_HasNext(buffer))
        if (!GNL_Buffer_Refresh(buffer))
            return false;
    *into = buffer->chars[buffer->read_head++];
    return true;
}

char* GNL_Buffer_GetNextLine(struct GNL_Buffer* buffer)
{
    struct GNL_StringBuilder  builder;
    char*                     result;
    char                      c;

    GNL_StringBuilder_Create(&builder);
    while (GNL_Buffer_ReadNext(buffer, &c))
    {
        GNL_StringBuilder_AppendChar(&builder, c);
        if (c == '\n')
            break;
    }
    GNL_StringBuilder_Finalize(&builder, &result);
    GNL_StringBuilder_Destroy(&builder);
    return result;
}


/*
 *    G e t N e x t L i n e
 */

char* get_next_line(int fd)
{
    static struct GNL_Buffer buffers[MAX_FD];

    if (fd < 0 || fd >= MAX_FD)
        return NULL;
    else if (buffers[fd].initialized == false)
        GNL_Buffer_Create(&buffers[fd], fd);
    return GNL_Buffer_GetNextLine(&buffers[fd]);
}


int main(int argc, char *argv[])
{
    char*  str;

    str = get_next_line(STDIN_FILENO);
    while (str != NULL)
    {
        printf("Line read: %s\n", str);
        free(str);
        str = get_next_line(STDIN_FILENO);
    }

    return 0;
}
