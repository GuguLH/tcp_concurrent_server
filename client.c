#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int fd = 0, cfd = 0, ret = 0;
    ssize_t rbytes = 0, wbytes = 0;
    // 1 建立tcp套接字
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("Error socket()");
        return -1;
    }

    // 2 连接端口
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    saddr.sin_port = htons(9999);
    ret = connect(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        perror("Error connect()");
        return -1;
    }

    // 3 和服务端通信
    int num = 0;
    while (1)
    {
        // 发送数据
        char buf[1024] = {0};
        sprintf(buf, "你好,服务器...%d\n", num++);
        wbytes = send(fd, buf, strlen(buf) + 1, 0);
        if (wbytes == -1)
        {
            perror("Error send()");
            break;
        }
        // 接收数据
        memset(buf, 0, sizeof(buf));
        rbytes = recv(fd, buf, sizeof(buf), 0);
        if (rbytes > 0)
        {
            printf("Server say: %s\n", buf);
        }
        else if (rbytes == 0)
        {
            printf("服务器断开了连接!\n");
            break;
        }
        else
        {
            perror("Error recv()");
            break;
        }
        sleep(1);
    }

    close(fd);
    return 0;
}