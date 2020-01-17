#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//http://zake7749.github.io/2015/03/17/SocketProgramming/
//unix network programming

int main() {

	//socket的建立
	char inputBuffer[256] = {};
	char message[] = {"Hi,this is server.\n"};
	int sockfd = 0, forClientSockfd = 0;
	sockfd = socket(AF_INET , SOCK_STREAM , 0);	
	//AF_INET : Internet (AF_INET)
	//SOCK_STREAM : stream (SOCK_STREAM) socket


	if(sockfd == -1) {
		printf("failed to create a socket.");
	}


	//socket的連線
	struct sockaddr_in serverInfo,clientInfo;
	int addrlen = sizeof(clientInfo);
 	bzero(&serverInfo,sizeof(serverInfo));

	serverInfo.sin_family = PF_INET;
	//Q: AF_INET v.s. PF_INET ?

	serverInfo.sin_addr.s_addr = INADDR_ANY;
	//INADDR_ANY表示我不在乎loacl IP是什麼，讓kernel替我決定就好
	//llows the server to accept a client connection on any interface, in case the [ Team LiB ]
	//	server host has multiple interfaces.
	//
	//inet_ntop就是用在這個地方
 
/*
IPv4 socket address structure
struct in_addr {
	in_addr_t s_addr; 	 32-bit IPv4 address 
				 network byte ordered
};
struct sockaddr_in {
	uint8_t sin_len;		length of structure (16) 
	sa_family_t sin_family;		AF_INET 
	in_port_t sin_port;		16-bit TCP or UDP port number
					network byte ordered
	struct in_addr sin_addr;
	char sin_zero[8];		unused 
};

*/


	serverInfo.sin_port = htons(50000);
	bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
	//把這個socketfd綁在某個IP的某個port上。

	listen(sockfd, 25);
	//By calling listen, the socket is converted into a listening socket, on which incoming connections from clients will be accepted by the kernel.
	//後面那個數字代表kernel最多會幫我們queue多少的連線。

	while(1){
		forClientSockfd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
		//Since the kernel is passed both the pointer and the size of what the pointer points to, it knows exactly how much data to copy from the process into the kernel

		//send(forClientSockfd,message,sizeof(message),0);
		recv(forClientSockfd,inputBuffer,sizeof(inputBuffer),0);
		printf("Get:%s\n",inputBuffer);
	}

	return 0;

/*
  雖然這樣可以簡單地進行傳訊
  但是假如有多個連線一起建立起來的話，就會分不清誰是誰了
  Q:有沒有辦法可以持續使用同一個fd呢？

  Q:有沒有辦法知道client已經close了呢？

 */

}
