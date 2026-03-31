#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "mychar_ioctl.h"

int main(void)
{
    int fd;
    int size;
    int new_limit = 64;
    char buf[128] = {0};

    fd = open("/dev/mychar", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (write(fd, "hello", 5) < 0) {
        perror("write hello");
        close(fd);
        return 1;
    }

    if (write(fd, " kernel", 7) < 0) {
        perror("write kernel");
        close(fd);
        return 1;
    }

    if (ioctl(fd, MYCHAR_IOCTL_GET_SIZE, &size) < 0) {
        perror("ioctl GET_SIZE");
        close(fd);
        return 1;
    }
    printf("Current size: %d\n", size);

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    if (read(fd, buf, sizeof(buf) - 1) < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    printf("Read back: %s\n", buf);

    if (ioctl(fd, MYCHAR_IOCTL_SET_LIMIT, &new_limit) < 0) {
        perror("ioctl SET_LIMIT");
        close(fd);
        return 1;
    }
    printf("Set new limit to %d\n", new_limit);

    if (ioctl(fd, MYCHAR_IOCTL_CLEAR) < 0) {
        perror("ioctl CLEAR");
        close(fd);
        return 1;
    }

    if (ioctl(fd, MYCHAR_IOCTL_GET_SIZE, &size) < 0) {
        perror("ioctl GET_SIZE after clear");
        close(fd);
        return 1;
    }
    printf("Size after clear: %d\n", size);

    close(fd);
    return 0;
}