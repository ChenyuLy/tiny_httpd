#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>//strcasecmp
#include <ctype.h> //isspace
#include <pthread.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //这个库有什么用

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void fill_adrr(struct sockaddr_in * addr,unsigned short  * port){

    memset(addr,0,sizeof(*addr));
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(*port);
    addr->sin_family = AF_INET;
    return ;
}
void error_die(const char *sc)
{
    perror(sc);

    exit(1);
}

int Make_Listen(short unsigned int *port)
{
    struct sockaddr_in server_listen_addr;
    fill_adrr(&server_listen_addr,port);
    int res = -1;
    socklen_t addr_len = sizeof(server_listen_addr);       // 创建套结字
    res = socket(AF_INET, SOCK_STREAM, 0); // 理论上建立socket时是指定协议，应该用PF_xxxx，设置地址时应该用AF_xxxx。当然AF_INET和PF_INET的值是相同的，混用也不会有太大的问题。
    if (res == -1)
        error_die("create socket fail ...\n");
    if (-1 == bind(res, (struct sockaddr *)&server_listen_addr, addr_len))
        error_die("ls bind fail ...:"); // 绑定套结字

    if (*port == 0)// 如果端口为0自动分配套结字
    { 
        if (getsockname(res, (struct sockaddr *)&server_listen_addr, &addr_len) == -1)
            error_die("getsocketname error ...\n");
        *port = ntohs(server_listen_addr.sin_port);
    }

    if (listen(res, 5) < 0)
        error_die("listen error"); // 套结字开始监听 允许5个连接排队连接

    printf("start to listen in ip:%s port:%d\n",inet_ntoa(server_listen_addr.sin_addr),*port);

    return res; // 返回创建好的文件描述符
}




int Accept(int server_fd,struct sockaddr_in *client_addr ,socklen_t * len){
    int res = accept(server_fd,(struct sockaddr * )client_addr,len);
    if (res == -1)
    {
        error_die("accept error ...\n");
    }

    printf("client:%s connected ...\n",inet_ntoa((*client_addr).sin_addr));
    return res;
} 


int get_line(int sock, char *buf,int size){     //读取一行 返回读取到的字符个数
    int i = 0;                                  
    char c ='\0';
    int n ;

    while (i<size -1 && c != '\n')
    {
        n = recv(sock,&c,1,0);
        if(n>0){
            if(c == '\r'){
                n = recv(sock,&c,1,MSG_PEEK);
                if (n> 0 &&  c == '\n') recv(sock,&c,1,0);
                else c = '\n';
            }
            buf[i] = c;
            i++;

        }
        else c = '\n';
    }

    buf[i] = '\0';
    return(i);
}


void not_found(int client)  //404
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "your request because the resource specified\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "is unavailable or nonexistent.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}
void unimplemented(int client)          //没有对应的方法
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</TITLE></HEAD>\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "</BODY></HTML>\r\n");
 send(client, buf, strlen(buf), 0);
}

void header(int client,const char * filename){
    char buf[1024];
    (void)filename;//用filename 调用stat等操作

    strcpy(buf,"HTTP/1.0 200 OK\r\n");
    send(client,buf,strlen(buf),0);
    strcpy(buf,SERVER_STRING);
    send(client,buf,strlen(buf),0);
    strcpy(buf,"Content-Type: text/html\r\n");
    send(client,buf,strlen(buf),0);
    strcpy(buf,"\r\n");
    send(client,buf,strlen(buf),0);

}

void cat(int client,FILE * resource){
    char buf[1024];
    //fgets 从文件里度一行，遇到换行符eof error 终止；
    fgets(buf,sizeof(buf),resource);

    while(!feof(resource)){
        send(client,buf,strlen(buf),0);
        fgets(buf,sizeof(buf),resource);
    }
}
//向客户端发送文件函数
void serve_file(int client,const char *filename){
    FILE *resource = NULL;
    int numchars = 1 ;
    char buf[1024];
    
    buf[0] = 'A' ; buf[1] ='\0';
    //不需要头部的信息足行读取丢弃
    while(numchars > 0 && strcmp("\n",buf))
        numchars = get_line(client,buf,sizeof(buf));
    
    resource = fopen(filename,"r");
    // printf("%d",resource  == NULL);
    if(resource  == NULL){
        not_found(client); //没有文件发送失败           //fopen 换成open可以吗 
    }

    else{
        header(client,filename);    //向客户端发送请求正确头
        cat(client,resource);       //诸行将文件发送到客户端 

    }
    fclose(resource);
}

void cannot_execute(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
 send(client, buf, strlen(buf), 0);
}
void bad_request(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "Content-type: text/html\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "\r\n");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "<P>Your browser sent a bad request, ");
 send(client, buf, sizeof(buf), 0);
 sprintf(buf, "such as a POST without a Content-Length.\r\n");
 send(client, buf, sizeof(buf), 0);
}
void execute_cgi(int client ,const char * path,const char *method,const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid ;
    int status ;
    int i;
    char c;
    int numchars = 1;
    int content_length;

    buf[0] = 'A'; buf[1] = '\0';
    if(strcasecmp(method,"GET") == 0){ //get方法不需要头
        while (numchars > 0  && strcmp("\n", buf)) //一行一行扔
        {
             numchars =get_line(client,buf,sizeof(buf)); 
        }
    }
    else{  //post               //取Content-Length 扔掉其他的
        numchars = get_line(client, buf, sizeof(buf));
        while (numchars > 0 && strcmp("\n",buf))
        {
            buf[15] = '\0';
            if(strcasecmp(buf,"Content-Length:") == 0) content_length =atoi(&(buf[16]));
            numchars = get_line(client,buf,sizeof(buf));
        }

        if(content_length == -1){
            bad_request(client);   //发送错误消息给客户端
            return;
            }
        }

    sprintf(buf,"HTTP/1.0 200 OK\r\n");
    send(client,buf,strlen(buf),0);

    if (pipe(cgi_output) < 0)  //创建管道
    {
        cannot_execute(client);
        return;
    }
    if(pipe(cgi_input)<0){
        cannot_execute(client);
        return;
    }
    
    if ( (pid = fork()) < 0 ) {   //创建进程
        cannot_execute(client);
        return;
    }

    if (pid == 0) /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], 1); //cgi_output[1]指向了标准输出 [0]指向了标准输入
        dup2(cgi_input[0], 0);
        close(cgi_output[0]); //关闭输入端的读
        close(cgi_input[1]); //关闭读如段的写
        sprintf(meth_env, "REQUEST_METHOD=%s", method); //把http方法写入环境变量
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0)
        {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else
        { /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, path, NULL);
        exit(0);
    }
    else
    { /* parent */
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++)
            {
                recv(client, &c, 1, 0);
                fprintf(stderr,"%c\n",c);
                write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}



void * runclient(int *client){
    char buf[1024];
    int numchars;
    char method[255];
    char url[255];
    char path[512];
    size_t i,j;
    struct stat st;  //用来干吗的？记录文件的状态
    int cgi = 0; //如果为真表示，服务器认为这是一个cgi程序
    char *query_string = NULL;

    pthread_detach( pthread_self());
    // printf("%d\n",*client);
    numchars = get_line(*client,buf,sizeof(buf)); //读取客户端传来一行数据
    i = 0;j= 0;

    while (!ISspace(buf[j]) && i<sizeof(method)-1 )  //如果buf 位不是空格并且不是method 中的最后一位   读取空格分割符前的http方法
    {                                                //将buf的非空格传给method,直到填满method或者 buf没有
        method[i] = buf[j];
        i++;j++;
    }
    method[i] = '\0';
    
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))  //strcasecmp 忽略大小写比较字符串 
    {
        unimplemented(*client);          //向客户端发送错误错误网页
        return NULL;                         //如果方法中没有get post方法结束线程
    }
    if (strcasecmp(method, "POST") == 0)  cgi = 1;//如果方法为post 认为这是一个cgi程序

    i = 0 ;
    while (ISspace(buf[j]) && (j < sizeof(buf))) j++; //调整j的位置到下一个不为空格的数据上
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))  //读取第一行的第二项 url的数据
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';



    if (strcasecmp(method, "GET") == 0){  //GET方法会在url后面拼接数据且标识符为？
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0')) query_string++;
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++; //指向问号后面的数据
        }

    }

    // sprintf(path, "%s", url);//如果url是一个目录的话，就把该目录下的index.html文件返回
    sprintf(path, "htdocs%s", url);//如果url是一个目录的话，就把该目录下的index.html文件返回/

    if (path[strlen(path) - 1] == '/') strcat(path, "index.html"); //查看这个目录是否存在
    printf("request url : %s\n",path);
    // printf("cgi : %d\n",cgi);
    if (stat(path, &st) == -1){  //stat获取文件的信息把他存到st中
        // printf("+++\n");
        error_die("stat:");
        while ((numchars > 0) && strcmp("\n", buf)) numchars = get_line(*client, buf, sizeof(buf));//读取并丢弃header
        not_found(*client);                      //返回报错
    }
    else
    {
        // printf("---/n");
        if ((st.st_mode & __S_IFMT) == __S_IFDIR) strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) ||(st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) cgi = 1; //如果都有执行权限
        if (!cgi) serve_file(*client, path); //如果没有cgi标识说明是请求了一个文件，不然就是cgi   // 把文件发送给客户端
        else {
            // printf("---------------\n");
            // printf("%s---------------\n",query_string);
            execute_cgi(*client, path, method, query_string);}

    }


    close(*client);
    return NULL;
}


int main(int argc, char const *argv[])
{
    int server_socket = -1; // 用来返回服务器监听套结字的文将描述符
    unsigned short port = 0;
    pthread_t tid; // 建立连接后创建接收返回的线程

    if (argc == 2 ){
        port = atoi(argv[1]);
    }
        
    server_socket = Make_Listen(&port);
    // printf("httpd running on port %d \n",port);


    struct sockaddr_in client_addr;
    socklen_t client_addr_len =sizeof(client_addr);



    while (1)
    {
        // printf("whiling listening\n");
        int client_socket = Accept(server_socket,&client_addr,&client_addr_len); //连接

        // if (pthread_create(&tid,NULL,(void *)runclient,(void *)&client_socket) != 0 )
        if (pthread_create(&tid,NULL,(void *)runclient,&client_socket) != 0 )
        {
            perror("pthread_create fail ...\n");
        }
        // printf("create thread:%ld\n",tid);

    }

    close(server_socket);
    


    return 0;
}
