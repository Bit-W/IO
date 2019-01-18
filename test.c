/*  这是一个验证描述符阻塞/非阻塞的一个demo
 *      普通文件无法验证，因为文件要是没有数据就返回了
 *      使用管道文件来验证，因为管道如果以只读打开，要是没有数据就
 *      会阻塞。
 *      int fcntl(int fd, int cmd, ...);
 *      cmd:O_NONBLOCK
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
int main()
{
    char *ptr = "./test.fifo";
    mkfifo(ptr, 0664);

    int fd = open(ptr, O_WRONLY);
    if (fd < 0) {
        perror("open error");
        return -1;
    }

    sleep(30);
    close(fd);
    return 0;
}
