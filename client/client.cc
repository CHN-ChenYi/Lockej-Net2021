/*
 *	本程序的主要目的在于说明socket编程的基本过程，所以服务器/客户端的交互过程非常简单，
 *  只是由客户端向服务器传送一个学生信息的结构。
 */
//informLinuxClient.cpp：参数为 serverIP name age
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../MsgDef.hpp"

using namespace std;

extern int errno;

//客户端向服务器传送的结构：
struct student
{
	char name[100];
	int age;
};

int main(int argc, char *argv[])
{
	int sockfd;
	int option;
	unsigned msg_type;
	string ipaddr;
	int port;
	struct sockaddr_in serverAddr;
	struct student stu;

	while (1)
	{
		cout << "please enter service number: \n-1 Connect Server\n-2 Close Connection\n-3 Get Time\n-4 Get Name\n-5 Get List\n-6 Send Message\n-7 Quit\n";
		cin >> option;

		switch (option)
		{
		case 1:
			cout << "please enter IP address:" << endl;
			cin >> ipaddr;
			cout << "please enter port number:" << endl;
			cin >> port;

			// 创建一个socket：
			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				printf("socket() failed! code:%d\n", errno);
				return -1;
			}

			// 设置服务器的地址信息：
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_port = htons(port);
			serverAddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
			bzero(&(serverAddr.sin_zero), 8);

			//客户端发出连接请求：
			printf("Connecting to Server %s : %d\n", ipaddr.c_str(), port);
			if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
			{
				printf("connect() failed! code:%d\n", errno);
				close(sockfd);
				return -1;
			}

			printf("Connected!\n");
			break;

		case 2:
			//关闭socket
			msg_type = static_cast<unsigned>(MsgType::kDisconnect);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error, close locally." << endl;			
			close(sockfd);
			cout << "Connection to " << ipaddr << " : " << port << " is closed! \n"
				 << endl;
			break;

		case 3:
			//获取时间

		case 7:
			cout << "Bye bye~\n\n";
			return 0;
		default:
			break;
		}

		// strcpy(stu.name, argv[2]);
		// stu.age = atoi(argv[3]);
		// if (send(sockfd, (char *)&stu, sizeof(stu), 0) == -1)
		// {
		// 	printf("send() failed!\n");
		// 	close(sockfd);
		// 	return -1;
		// }

		// printf("student info has been sent!\n");
		// close(sockfd); //关闭套接字
		// return 0;
	}
}
