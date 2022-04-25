#include "common.h"
#include "tinyHttp.h"

#define SBUFSIZE 16  //sbuf缓冲区大小
#define INT_THREAD_N 1  //初始线程数量
#define THREAD_LIMIT 512  //线程数量限制

int nthreads;
sbuf_t sbuf;
// info of threads
typedef struct {
    pthread_t tid;
    sem_t mutex;
}ithread;

ithread threads[THREAD_LIMIT];

void init();
void* server_thread(void *arg);
void* adjust_thread(void *arg);
void create_thread(int from,int to);

int main(int argc, char **argv)
{
    pthread_t tid;
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    if(signal(SIGCHLD,sigchild_handler)==SIG_ERR)
    {
        unix_error("sigal child handler error");
    }

    if(signal(SIGPIPE,SIG_IGN)==SIG_ERR)
    {
        unix_error("signal pipe error");
    }

    listenfd = make_socket();

    init();
    pthread_create(&tid,NULL,adjust_thread,NULL);

    printf("start server....\n");
    while (1) {

        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf,connfd);
    }
}

void init()
{
    nthreads=INT_THREAD_N;
    sbuf_init(&sbuf,SBUFSIZE);

    create_thread(0,nthreads);
}

void *server_thread(void *arg)
{
    int index=*(int*)arg;
    free(arg);

    while(1)
    {
        P(&threads[index].mutex);
        int connfd=sbuf_remove(&sbuf);
        doit(connfd);
        close(connfd);
        V(&threads[index].mutex);
    }
}

void create_thread(int from,int to)
{
    for(int i=from;i<to;i++)
    {
        Sem_init(&(threads[i].mutex),0,1);
        int *arg = (int*)malloc(sizeof(int));
        *arg=i;
        pthread_create(&(threads[i].tid),NULL,server_thread,arg);
    }
}

void* adjust_thread(void *arg)
{
    sbuf_t *sp=&sbuf;

    while(1)
    {
        //如果缓冲满
        if(sbuf_is_full(sp))
        {
            if(nthreads==THREAD_LIMIT){
                fprintf(stderr,"too many threads,can't create more\n");
                continue;
            }

            int dn=2*nthreads;
            create_thread(nthreads,dn);
            nthreads=dn;
            continue;
        }

        //缓冲空
        if(sbuf_is_empty(sp))
        {
            if(nthreads==1){
                continue;
            }

            int hn=nthreads/2;
            //kill [hn~ntheads] threads
            for(int i=hn;i<nthreads;i++)
            {
                P(&(threads[i].mutex));
                pthread_cancel(threads[i].tid);
                V(&(threads[i].mutex));
            }
            nthreads=hn;
            continue;
        }
    }
}
