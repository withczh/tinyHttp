#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 8192
#define MAXBUF 8912

#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* 与缓冲区绑定的文件描述符 */
    int rio_cnt;               /* 缓冲区中还未读取的字节数 */
    char *rio_bufptr;          /* 下一个未读取字符的地址 */
    char rio_buf[RIO_BUFSIZE]; /* 内部缓冲区 */
} rio_t;

//有限的缓冲区（先进先出队列模型），存放连接描述符供消费者线程使用
typedef struct {
    int *buf;
    int rear,front,size;    //size为资源的数量
    unsigned int capacity; //最多的坑位
    sem_t mutex;    //保护缓存区
    sem_t slots;    //坑位
    sem_t items;    //可获得的资源（描述符）
}sbuf_t;

extern char **environ;

//出错处理
void unix_error(char *msg);
//建立监听描述符
int make_socket();

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd); 
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
/*Rio package end*/

/*P AND V operation*/
void Sem_init(sem_t *sem, int pshared, unsigned int value);
void P(sem_t *sem);
void V(sem_t *sem);
/*end P V*/


/*有限缓存区的构造*/
void sbuf_init(sbuf_t *sp,int n);
//清空sbuf
void sbuf_deinit(sbuf_t *sp);
//添加描述符到队尾
void sbuf_insert(sbuf_t *sp,int item);
//从对头取出描述符
int sbuf_remove(sbuf_t *sp);
//判满
int sbuf_is_empty(sbuf_t *sp);
//判空
int sbuf_is_full(sbuf_t *sp);


#endif