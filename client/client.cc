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

int main(int argc, char *argv[])
{
	int sockfd;
	int option;
	unsigned msg_type;
	string ipaddr;
	int port;
	struct sockaddr_in serverAddr;

	while (1)
	{
		cout << "please enter service number: \n-1 Connect Server\n-2 Close Connection\n-3 Get Time\n-4 Get Name\n-5 Get List\n-6 Send Message\n-7 Quit\n";
		cin >> option;

		switch (option)
		{
		case 1:
		{
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
		}
		case 2:
		{
			//关闭socket
			msg_type = static_cast<unsigned>(MsgType::kDisconnect);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error, close locally." << endl;
			close(sockfd);
			cout << "Connection to " << ipaddr << " : " << port << " is closed! \n"
				 << endl;
			break;
		}
		case 3:
		{
			//获取时间
			time_t cur_time;
			struct tm *timestr;
			msg_type = static_cast<unsigned>(MsgType::kTime);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
			else
			{
				recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
				recv(sockfd, reinterpret_cast<void *>(&cur_time), sizeof(cur_time), 0);
				const MsgType msg_type_ = static_cast<MsgType>(msg_type);
				const time_t cur_time_ = static_cast<time_t>(cur_time);
				if (msg_type_ != MsgType::kTime)
					cout << "Recieve Type Error! Code: " << static_cast<int>(msg_type_) << endl;
				else
				{
					timestr = localtime(&cur_time_);
					printf("Current Time is: %s", asctime(timestr));
				}
			}
			break;
		}
		case 4:
		{
			string Hname;
			size_t name_len;
			msg_type = static_cast<unsigned>(MsgType::kHostname);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
			else
			{
				recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
				recv(sockfd, reinterpret_cast<void *>(&name_len), sizeof(name_len), 0);
				Hname.resize(name_len);
				recv(sockfd, reinterpret_cast<void *>(Hname.data()), sizeof(char) * name_len, 0);
				const MsgType msg_type_ = static_cast<MsgType>(msg_type);
				if (msg_type_ != MsgType::kHostname)
					cout << "Recieve Type Error! Code: " << static_cast<int>(msg_type_) << endl;
				else
				{
					cout << "HostName is: " << Hname << endl;
				}
			}
			break;
		}
		case 5:
		{
			size_t list_len;
			int fd;
			sockaddr_in addr;
			char addr_str[INET_ADDRSTRLEN];
			msg_type = static_cast<unsigned>(MsgType::kList);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
			else
			{
				recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
				recv(sockfd, reinterpret_cast<void *>(&list_len), sizeof(list_len), 0);
				const MsgType msg_type_ = static_cast<MsgType>(msg_type);
				if(msg_type_ != MsgType::kList)
					cout << "Recieve Type Error! Code: " << static_cast<int>(msg_type_) << endl;
				else
				{
					for(size_t i = 0; i<list_len; i++, cout << endl)
					{
						recv(sockfd, reinterpret_cast<void *>(&fd), sizeof(fd), 0);
						recv(sockfd, reinterpret_cast<void *>(&addr), sizeof(addr), 0);
						inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
						cout << "Socket fd " << fd << " Host Address " << addr_str << " : " << addr.sin_port; 
					}
				}
			}
			break;
		}
		case 7:
			cout
				<< "Bye bye~\n\n";
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
