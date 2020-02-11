#include "mdv_rand.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


void mdv_random(uint8_t *buf, size_t size)
{
    size_t gen_len = 0;

    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);

    if(fd != -1)
    {
        while(gen_len < size)
        {
            ssize_t ret = read(fd, buf + gen_len, size - gen_len);

            if(ret <= 0)
                break;

            gen_len += ret;
        }

        close(fd);
    }

    for(; gen_len < size;)
    {
        int const n = rand();

        size_t const len = sizeof(int) < size - gen_len
                            ? sizeof(int)
                            : size - gen_len;

        memcpy(buf + gen_len, &n, len);

        gen_len += len;
    }
}
