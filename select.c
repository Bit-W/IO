/*  这是一个使用了select多路转接模型的tcp服务端程序
 *  这个程序要实现阻塞的多客户端连接处理,打破之前单线程无法同时处理
 *  多个客户端数据的壁垒。
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    //实现tcp的服务端程序
    int sockfd = -1, ret;
    socklen_t len = -1;
    char buff[1024] = {0};
    struct sockaddr_in srv_addr;
    struct sockaddr_in cli_addr;
    //定义select描述符集合----读事件
    fd_set rds;
    //专门用于存储我们所有的描述符
    int fd_arry[1024] = {0};
    int max_fd = -1;
    int i, j;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket error");
        return -1;
    }
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(atoi(argv[2]));
    srv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    len = sizeof(struct sockaddr_in);
    ret = bind(sockfd, (struct sockaddr*)&srv_addr, len);
    if (ret < 0) {
        perror("bind error");
        return -1;
    }
    if (listen(sockfd, 5) < 0) {
        perror("listen error");
        return -1;
    }
    //将数组中的元素全部置位-1
    for (i = 0; i < 1024; i++) {
        fd_arry[i] = -1;
    }

    //将监听描述符添加到数组中
    fd_arry[0] = sockfd;
    while(1) {
        max_fd = sockfd;
        //不管集合中有什么数据，我们都清空重新添加描述符
        FD_ZERO(&rds);
        //将数组中不是-1的元素添加的select监控集合
        for (i = 0; i < 1024; i++) {
            if (fd_arry[i] != -1) {
                FD_SET(fd_arry[i], &rds);
                max_fd = max_fd > fd_arry[i]? max_fd:fd_arry[i];
            }
        }
        //关数据造成影响，因此首要任务是清空集合
        //将监听描述符添加到读事件集合
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        ret = select(max_fd+1, &rds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select error");
            continue;
        }else if (ret == 0) {
            printf("have no data arrived!!\n");
            //可以做一些超时相关事情---长时间不发数据过来
            continue;
        }
        //select一旦返回，并且不是超时和出错，那么这时候留在集合中
        //的都是就绪的描述符，(这个集合每次都会被修改，因此描述符
        //每次都需要重新添加)
        //ret > 0  表示当前的这些集合中有多少个描述符就绪了
        for (i = 0 ; i < max_fd+1; i++) {
            //判断到底哪个描述符还在集合中,留在集合中的描述符是就绪描述符
            if (FD_ISSET(i, &rds)) {
                //判断这个描述符是不是监听描述符
                if (i == sockfd) {
                    //代表就绪的描述符是监听描述符，有新的客户端连接请求到来
                    int new_fd;
                    new_fd = accept(sockfd, 
                            (struct sockaddr*)&cli_addr, &len);
                    if (new_fd < 0) {
                        perror("accept error");
                        continue;
                    }
                    printf("have new connect\n");
                    //将新的描述符添加到监控集合中
                    for (j = 0; j < 1024; j++) {
                        if (fd_arry[j] == -1) {
                            fd_arry[j] = new_fd;
                            max_fd = max_fd > new_fd ? max_fd:new_fd;
                            break;
                        }
                    }
                }else {
                    //代表就绪的描述符不是监听描述符，那么就接收数据
                    memset(buff, 0x00, 1024);
                    ret = recv(i, buff, 1023, 0);
                    if (ret <= 0) {
                        if (errno == EAGAIN || errno == EINTR) {
                            continue;
                        }
                        perror("recv error");
                        close(i);
                        for (j = 0; j < 1024; j++) {
                            if (i == fd_arry[j]) {
                                fd_arry[j] = -1;
                            }
                        }
                    }
                    printf("client say:%s\n", buff);
                }
            }
        }
        
        //客户端数据处理
    }
    close(sockfd);
    return 0;
}
