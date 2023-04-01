#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>



void error_die(const char *sc)
{
    perror(sc);

    exit(1);
}

int Make_Listen(int *port)
{
    struct sockaddr_in server_listen_addr;
    fill_adrr(&server_listen_addr,port);
    int res = -1;
    socklen_t addr_len = len(&server_listen_addr);       // 创建套结字
    res = socket(AF_INET, SOCK_STREAM, 0); // 理论上建立socket时是指定协议，应该用PF_xxxx，设置地址时应该用AF_xxxx。当然AF_INET和PF_INET的值是相同的，混用也不会有太大的问题。
    if (res == -1)
        error_die("create socket fail ...\n");
    if (-1 == bind(res, (struct sockaddr *)&server_listen_addr, addr_len))
        error_die("ls bind fail ...\n"); // 绑定套结字

    if (*port == 0)// 如果端口为0自动分配套结字
    { 
        if (getsockname(res, (struct sockaddr *)&server_listen_addr, &addr_len) == -1)
            error_die("getsocketname error ...\n");
        *port = ntohs(server_listen_addr.sin_port);
    }

    if (listen(res, 5) < 0)
        error_die("listen error"); // 套结字开始监听 允许5个连接排队连接

    return res; // 返回创建好的文件描述符
}

void fill_adrr(struct sockaddr_in * addr,int  * port){

    memset(addr,0,sizeof(*addr));
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(*port);
    addr->sin_family = AF_INET;
}


int Accept(int server_fd,struct sockaddr_in *client_addr ,socklen_t * len){
    int res = accept(server_fd,client_addr,len);
    if (res == -1)
    {
        error_die("accept error ...\n");
    }
    return res;
}

void runclient(int tid){
    
}


int main(int argc, char const *argv[])
{
    int server_socket = -1; // 用来返回服务器监听套结字的文将描述符
    unsigned short port = 0;
    pthread_t tid; // 建立连接后创建接收返回的线程

    if (argc == 2 ){
        port = argv[1];
    }
        
    server_socket = Make_Listen(port);
    printf("httpd running on port %d \n",port);


    struct sockaddr_in client_addr;
    socklen_t client_addr_len =sizeof(client_addr);


    while (1)
    {
        int client_socket = Accept(server_socket,&client_addr,&client_addr_len); //连接

        if (pthread_create(&tid,NULL,runclient,client_socket))
        {
            perror("pthread_create fail ...\n");
        }
    }

    close(server_socket);
    


    return 0;
}
