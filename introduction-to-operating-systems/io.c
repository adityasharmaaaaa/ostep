#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void)
{
    int fd = open("/tmp/file", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    assert(fd > -1);

    /* 13 bytes includes the string's terminating null byte. */
    int rc = (int)write(fd, "hello world\n", 13);
    assert(rc == 13);

    close(fd);
    return 0;
}
