//////////////////////////////////////////////////////
//	File Name : client.c                        //
//      Date      : 2018.04.26                      //
//      Os        : Ubuntu 16.04 LTS 64 bits        //
//      Author    : Park Siyeon                     //
//      Student ID: 2016722094                      //
//////////////////////////////////////////////////////

//  Title: System Programming Assignment#2-1
//  Description
//     client sends url to server by using socket
//     client recives string that represent hit or miss




#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFSIZE 1024
#define PORTNO   40000

int main()
{
	int socket_fd, len;
	struct sockaddr_in server_addr;
	char haddr[]="127.0.0.1";
	char buf[BUFFSIZE];

	if( (socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1 ){
		printf("Can't create socket.\n");
		return -1;
	}

	bzero( buf, sizeof(buf));
	bzero( (char*)&server_addr, sizeof(server_addr) );

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(haddr);
	server_addr.sin_port = htons(PORTNO); 			// make socket information

	if( connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){ //connect
		printf("can't connect \n");
		return -1;
	}
	
	while(1)
	{
		printf("> Input url:");
		scanf("%s",buf);
		
		write(socket_fd,buf,BUFFSIZE);//send
	
		if(strcmp(buf,"bye") == 0)//bye= quit
				break;
		read(socket_fd,buf,BUFFSIZE);// recive
		printf("%s\n",buf);
	}
	close(socket_fd);
	return 0;
}

