#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>

 int main(int argc, char const *argv[])
 {
    char *data;
    char *length;
    char color[20];
    char c = 0 ;
    int flag = -1;

    std::cout<< "Content-Type: text/html\r\n"<<std::endl;
    std::cout<< "<HTML><head> <link rel=\"icon\" href=\"data:,\"> <title>color world</title> </head>"
                "<h1>Hello guys</h1>"
                "<BODY><P>The page color is :"<<std::endl;

    if((data = getenv("QUERY_STRING")) != NULL){
        while(*data != '='){
            data++;
        data++;
        sprintf(color ,"%s",data);
        }        
    } 

    if((length = getenv("CONTENT_LENGTH")) != NULL){
        int i ;
        for(i =0;i<atoi(length);i++){
            read(STDIN_FILENO,&c,1);
            if ( c == '=')  
            {
                flag = 0;
                continue;
            }
            if(flag > -1){
                color[flag++] = c;
            }
        }
        color[flag] = '\0'; 
    }

    std::cout << color <<std::endl;
    std::cout << "<body bgcolor =\""<< color <<"\"/>"<<std::endl;

    std::cout << "</BODY></HTML>"<< std::endl;
    return 0;
 }
 