/* 这是一个使用了epoll多路转接技术的tcp服务端
 * 使用epoll来打破原先单线程只能处理一个客户端的壁垒
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

void setnonblock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
int recv_data(int fd, char *buff)
{
    int ret;
    int alen = 0;
    while(1) {
        //边缘触发，为了防止缓冲区数据没有读完，因此我们进行了循环
        //读取，但是循环读取存在一个问题：recv有可能会阻塞
        ret = recv(fd, buff + alen, 2, 0);
        if (ret <= 0) {
            if (errno == EAGAIN) {
                //没数据了,跳出
                break;
            }else if (errno == EINTR) {
                //被信号打断，重新接收数据
                continue;
            }
            return -1;
        }else if (ret < 2) {
            //如果接收的数据小于想要获取的长度，代表数据读完了
            break;
        }
        alen += ret;

    }
}

int main(int argc, char *argv[])
{
    //实现tcp的服务端程序
    int sockfd = -1, ret, i;
    socklen_t len = -1;
    char buff[1024] = {0};
    struct sockaddr_in srv_addr;
    struct sockaddr_in cli_addr;
//创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket error");
        return -1;
    }
//绑定地址
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(atoi(argv[2]));
    srv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    len = sizeof(struct sockaddr_in);
    ret = bind(sockfd, (struct sockaddr*)&srv_addr, len);
    if (ret < 0) {
        perror("bind error");
        return -1;
    }
//监听
    if (listen(sockfd, 5) < 0) {
        perror("listen error");
        return -1;
    }
    //在内核创建eventpoll结构
    //int epoll_create(int size);
    //  size:   现在已经被忽略了，大于0即可
    //  返回：epoll句柄（文件描述符）
    int epfd = epoll_create(10);
    if (epfd < 0) {
        perror("epoll_create error");
        return -1;
    }
    //向内核添加事件
    //int epoll_ctl(int epfd, int op, int fd, 
    //  struct epoll_event *event);
    //  epfd:   epoll句柄
    //  op：    当前操作
    //      EPOLL_CTL_ADD   添加事件
    //      EPOLL_CTL_MOD   事件修改
    //      EPOLL_CTL_DEL   删除事件
    //  fd：    所监控的描述符
    //  event： 跟描述符所关联的事件
    //      EPOLLIN     可读事件
    //      EPOLLOUT    可写事件
    //      EPOLLET     边缘触发属性
    //          每当有新的数据到来的时候，事件会触发一次，假如缓冲
    //          区数据没有读完，但是不会再次提醒（就这一条数据来说
    //          不会再次触发事件）
    //          基于这种特性，就需要我们一次性将缓冲区数据全部读完
    //      EPOLLLT     水平触发（默认）
    //          如何说明一个描述符是读就绪的？
    //          可读事件就是缓冲区是否有数据可读
    //          只要缓冲区有数据就一直提醒（每次epoll都会触发事件就
    //          绪）
    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

//注册事件
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while(1) {
        struct epoll_event evs[10];
        //int epoll_wait(int epfd, struct epoll_event *events,
        //  int maxevents, int timeout);
        //  epfd：  epoll句柄
        //  events：epoll_event结构体数组，用于获取就绪的描述符事件
        //          这个事件就是我们之前添加时候描述符关联的事件
        //  maxevents： 能够获取的最大就绪事件个数（等同数组大小）
        //  timeout：epoll最大超时等待时间（毫秒）
        //  返回：错误：-1   超时：=0   就绪个数:>0
        int nfds = epoll_wait(epfd, evs, 10, 3000);

        if (nfds < 0) {
            perror("epoll_wait error");
            return -1;
        }else if (nfds == 0) {
            printf("have no data arrived!!\n");
            continue;
        }
        //对就绪的时间进行处理
        for (i = 0; i < nfds; i++) {
            //如果就绪的时间中对应的描述符是监听描述符
            if (evs[i].data.fd == sockfd) {

                int new_fd;
                new_fd = accept(sockfd, 
                        (struct sockaddr*)&cli_addr, &len);
                if (new_fd < 0) {
                    perror("accept error");
                    continue;
                }
                //针对边缘触发模式的描述符需要设置为非阻塞
                //setnonblock(new_fd);
                //给新的连接，组装一个事件结构，并且假如到epoll的内
                //核事件监控集合
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = new_fd;

                epoll_ctl(epfd, EPOLL_CTL_ADD, new_fd, &ev);
            }else {
                char buff[1024] = {0};
                ret = recv_data(evs[i].data.fd, buff);
                if (ret <= 0) {
                    if (errno == EAGAIN||errno == EINTR) {
                        continue;
                    }
                    epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd,
                            &ev);
                    close(evs[i].data.fd);
                }
                printf("client say:%s\n", buff);
            }
        }
        //客户端数据处理
    }
    close(sockfd);
    return 0;
}
