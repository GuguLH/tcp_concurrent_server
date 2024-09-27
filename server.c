#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "thread_pool.h"

typedef struct sock_info
{
    struct sockaddr_in addr;
    int fd;
} sock_info_t;

typedef struct pool_info
{
    thread_pool_t *p;
    int fd;
} pool_info_t;

void do_accept(void *arg);
void do_work(void *arg);

int main()
{
    int fd = 0, cfd = 0, ret = 0;
    // 1 建立tcp套接字
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("Error socket()");
        return -1;
    }

    // 2 绑定端口
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    // INADDR_ANY代表本机的所有IP, 假设有三个网卡就有三个IP地址
    // 这个宏可以代表任意一个IP地址
    // 这个宏一般用于本地的绑定操作
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(9999);
    ret = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        perror("Error bind()");
        return -1;
    }

    // 3 开始监听
    ret = listen(fd, 128);
    if (ret == -1)
    {
        perror("Error listen()");
        return -1;
    }

    // 初始化线程池
    thread_pool_t *pool = thread_pool_create(3, 8, 100);
    // 添加接收客户端套接字任务到线程池
    pool_info_t *p_info = (pool_info_t *)malloc(sizeof(pool_info_t)); // 因为线程池在工作线程执行完毕会进行堆区内存回收,所以堆区分配空间
    p_info->p = pool;
    p_info->fd = fd;
    thread_pool_add_task(pool, do_accept, (void *)p_info);

    // 主线程任务结束,退出
    pthread_exit(NULL);
    return 0;
}

void do_accept(void *arg)
{
    pool_info_t *p_info = (pool_info_t *)arg;
    int slen = sizeof(struct sockaddr_in);
    // 4 阻塞并等待接收客户端套接字
    while (1)
    {
        int cfd = 0;
        // 堆区分配空间
        sock_info_t *sock_info = (sock_info_t *)malloc(sizeof(sock_info_t));
        cfd = accept(p_info->fd, (struct sockaddr *)&sock_info->addr, &slen);
        if (cfd == -1)
        {
            perror("Error accept()");
            break;
        }

        sock_info->fd = cfd;
        // 工作线程交由线程池
        thread_pool_add_task(p_info->p, do_work, (void *)sock_info);
    }

    close(p_info->fd);
}

void do_work(void *arg)
{
    ssize_t rbytes = 0, wbytes = 0;
    sock_info_t *info = (sock_info_t *)arg;
    // 打印客户端的地址信息
    char ip[32] = {0};
    printf("客户端IP: %s, Port: %d\n", inet_ntop(AF_INET, &(info->addr).sin_addr.s_addr, ip, sizeof(ip)), ntohs(info->addr.sin_port));

    // 5 和客户端通信
    while (1)
    {
        // 接收数据
        char buf[1024] = {0};
        rbytes = recv(info->fd, buf, sizeof(buf), 0);
        if (rbytes > 0)
        {
            printf("Client say: %s\n", buf);
            wbytes = send(info->fd, buf, strlen(buf) + 1, 0);
            if (wbytes != rbytes)
            {
                printf("Error send()\n");
                break;
            }
        }
        else if (rbytes == 0)
        {
            printf("客户端断开了连接!\n");
            break;
        }
        else
        {
            perror("Error recv()");
            break;
        }
    }

    close(info->fd);
}