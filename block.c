/*  这是一个验证描述符阻塞/非阻塞的一个demo
 *      普通文件无法验证，因为文件要是没有数据就返回了
 *      使用管道文件来验证，因为管道如果以只读打开，要是没有数据就
 *      会阻塞。
 *      int fcntl(int fd, int cmd, ...);
 *      cmd:
 *          F_GETFL 获取当前文件的属性状态
 *          F_SETFL 设置当前文件的属性状态
 *      O_NONBLOCK
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

void nonblocking(int fd)
{
    int flag;
    //先获取文件原有属性
    flag = fcntl(fd, F_GETFL, 0);
    //设置属性的时候，不能直接设置O_NONBLOCK，需要在原有属性基础上
    //添加O_NONBLOCK属性
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
int main()
{
    char *ptr = "./test.fifo";
    mkfifo(ptr, 0664);

    int fd = open(ptr, O_RDONLY);
    if (fd < 0) {
        perror("open error");
        return -1;
    }
    nonblocking(fd);

    char buff[1024] = {0};
    read(fd, buff, 1024);
    printf("buff:[%s]\n", buff);

    close(fd);
    return 0;
}
