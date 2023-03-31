#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<pthread.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


void error_die(const char * sc){
    perror(sc);

    exit(1);
}


int Make_Listen(struct sockaddr_in * addr,int * port){
    int res = -1;
    int addr_len = len(*addr);
                                            //创建套结字
    res = socket(AF_INET,SOCK_STREAM,0); //理论上建立socket时是指定协议，应该用PF_xxxx，设置地址时应该用AF_xxxx。当然AF_INET和PF_INET的值是相同的，混用也不会有太大的问题。
    if(res == -1){
        error_die("create socket fail ...\n");
    }    
    
    if( -1 == bind(res,(struct sockaddr *)addr,addr_len)){  //绑定套结字
        error_die("ls bind fail ...\n");
    }

    if(*port == 0){ //如果端口为0自动分配套结字
        if(getsockname())
    }



    


    return res;
}

int main(int argc, char const *argv[])
{
    int server_socket = -1;  //用来返回服务器监听套结字的文将描述符
    unsigned short port;
    struct sockaddr_in server_listen_addr;

    server_socket = Make_Listen();

    return 0;
}
