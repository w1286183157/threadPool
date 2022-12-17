#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "threadPool.h"
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#define PORT "1999"
#define BUFSIZE 2014
void taskFunc(void *arg)
{
    printf("thread %ld start work\n", pthread_self());
    char buf[BUFSIZE];
    int fd = *((int *)arg);
    while (1)
    {
        ssize_t rlen = read(fd, buf, BUFSIZE);
        if (rlen == 0)
        {
            close(fd);
            printf("close client %d\n", fd);
            break;
        }
        buf[rlen] = 0;
        write(fd, buf, rlen);
    }
}

int main()
{
    int sfd;
    sockaddr_in serv_addr;
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd > 0);

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(PORT));
    serv_addr.sin_family = AF_INET;

    int open = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &open, sizeof(open));
    if (bind(sfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        fprintf(stderr, "bind error");
        return 1;
    }

    if (listen(sfd, 10) == -1)
    {
        fprintf(stderr, "listen error");
        return 1;
    }

    socklen_t clen;
    sockaddr_in clnt_addr;
    clen = sizeof(clnt_addr);

    threadPool *pool;
    pool = pool->threadPoolCreate(5, 10);

    while (1)
    {
        int cfd = accept(sfd, (sockaddr *)&clnt_addr, &clen);
        if (cfd < 0)
        {
            continue;
        }
        int *new_fd = (int *)malloc(sizeof(int));
        *new_fd = cfd;
        pool->append(taskFunc, new_fd);
    }
}