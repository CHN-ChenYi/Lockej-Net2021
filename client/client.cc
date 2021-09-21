#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <future>
#include "../MsgDef.hpp"
#include "Queue.hpp"

using namespace std;

extern int errno;
int sockfd;
bool conn;
bool quit;
int Request(int option);
void RecvMsg();

ThreadSafeQueue<string> msgQ;

int main(int argc, char *argv[])
{
	string msg;
	int option;
	quit = false;
	conn = false;
	future<void> recvMsg = async(launch::async, RecvMsg);

	while (1)
	{
		// recvMsg.wait_for(chrono::milliseconds(1));
		if (msgQ.empty())
		{
			cout << "please enter service number: \n-1 Connect Server\n-2 Close Connection\n-3 Get Time\n-4 Get HostName\n-5 Get List\n-6 Send Message\n-7 Quit\n";
			cin >> option;
			if (!(Request(option) == -1))
				std::this_thread::sleep_for(10ms);
			else
			{
				return 0;
			}
		}
		else
		{
			msgQ.pop(msg);
			cout << msg << endl;
		}
	}
}

void RecvMsg()
{
	unsigned msg_type;
	int count;
	int fd;
	MsgType option;
	size_t name_len;
	string Hname;
	time_t cur_time;
	struct tm *timestr;
	string time_str;
	size_t list_len;
	sockaddr_in addr;
	char addr_str[INET_ADDRSTRLEN];
	string num;
	string temp;
	string port;
	size_t msg_len;
	int src;
	string msg_content;

	while (1)
	{

		ioctl(sockfd, FIONREAD, &count);
		if (quit)
			break;
		if (conn && count)
		{
			recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(unsigned), 0);
			option = static_cast<MsgType>(msg_type);
			switch (option)
			{
			case MsgType::kHostname:
			{
				recv(sockfd, reinterpret_cast<void *>(&name_len), sizeof(name_len), 0);
				Hname.resize(name_len);
				recv(sockfd, reinterpret_cast<void *>(Hname.data()), sizeof(char) * name_len, 0);
				msgQ.push("Your Hostname is: " + Hname + "\n");
				break;
			}
			case MsgType::kTime:
			{
				recv(sockfd, reinterpret_cast<void *>(&cur_time), sizeof(cur_time), 0);
				timestr = localtime(&cur_time);
				time_str = asctime(timestr);
				msgQ.push("Current Time is: " + time_str);

				break;
			}

			case MsgType::kList:
			{
				recv(sockfd, reinterpret_cast<void *>(&src), sizeof(src), 0);
				num = to_string(src);
				msgQ.push("Yourself is Host: " + num);
				recv(sockfd, reinterpret_cast<void *>(&list_len), sizeof(list_len), 0);
				for (size_t i = 0; i < list_len; i++, cout << endl)
				{
					recv(sockfd, reinterpret_cast<void *>(&fd), sizeof(fd), 0);
					recv(sockfd, reinterpret_cast<void *>(&addr), sizeof(addr), 0);
					inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
					num = to_string(fd);
					port = to_string(addr.sin_port);
					temp = "Socket fd: " + num + " Host Address: " + addr_str + ":" + port;
					msgQ.push(temp);
				}
				msgQ.push("\n");
			}

			case MsgType::kMsg:
			{
				recv(sockfd, reinterpret_cast<void *>(&src), sizeof(src), 0);
				recv(sockfd, reinterpret_cast<void *>(&msg_len), sizeof(msg_len), 0);
				msg_content.resize(msg_len);
				recv(sockfd, reinterpret_cast<void *>(msg_content.data()), sizeof(char) * msg_len, 0);
				num = to_string(src);
				temp = "Message from Host " + num + ": " + msg_content + "\n";
				msgQ.push(temp);
			}

			default:
				break;
			}
		}
	}
}

int Request(int option)
{
	// int option;
	unsigned msg_type;
	string ipaddr;
	static int port;
	struct sockaddr_in serverAddr;
	string temp;

	// std::atomic<bool> heart_beating = true;
	// std::future<void> alive = std::async(
	// 	std::launch::async,
	// 	[](int *fd, std::atomic<bool> *heart_beating)
	// 	{
	// 		unsigned msg_type = static_cast<unsigned>(MsgType::kHeartBeat);
	// 		while (*heart_beating)
	// 		{
	// 			std::this_thread::sleep_for(kHeartBeatInterval);
	// 			send(*fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
	// 		}
	// 	},
	// 	&sockfd, &heart_beating);

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
			cout << "socket() failed! code:" << errno << endl;
			return -1;
		}

		// 设置服务器的地址信息：
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
		bzero(&(serverAddr.sin_zero), 8);

		//客户端发出连接请求：
		cout << "Connecting to Server " << ipaddr.c_str() << ":" << port << endl;
		if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		{
			cout << "connect() failed! code:" << errno << endl
				 << endl;
			close(sockfd);
			break;
		}
		cout << "Connect Success!" << endl;
		conn = true;
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
		conn = false;
		break;
	}
	case 3:
	{
		//获取时间
		// time_t cur_time;
		// struct tm *timestr;
		msg_type = static_cast<unsigned>(MsgType::kTime);
		if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
			cout << "Communication Error! " << endl;
		else
		{
			// recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
			// recv(sockfd, reinterpret_cast<void *>(&cur_time), sizeof(cur_time), 0);
			// const MsgType msg_type_ = static_cast<MsgType>(msg_type);
			// const time_t cur_time_ = static_cast<time_t>(cur_time);
			// if (msg_type_ != MsgType::kTime)
			// 	cout << "Recieve Type Error! Code: " << static_cast<int>(msg_type_) << endl;
			// else
			// {
			// 	timestr = localtime(&cur_time_);
			// 	string time_str = asctime(timestr);
			// 	cout << "Current Time is: " << time_str << endl;
			// }
		}
		break;
	}
	case 4:
	{
		// string Hname;
		// size_t name_len;
		msg_type = static_cast<unsigned>(MsgType::kHostname);
		if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
			cout << "Communication Error! " << endl;
		else
		{
			// recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
			// recv(sockfd, reinterpret_cast<void *>(&name_len), sizeof(name_len), 0);
			// Hname.resize(name_len);
			// recv(sockfd, reinterpret_cast<void *>(Hname.data()), sizeof(char) * name_len, 0);
			// const MsgType msg_type_ = static_cast<MsgType>(msg_type);
			// if (msg_type_ != MsgType::kHostname)
			// 	cout << "Recieve Type Error! Code: " << static_cast<int>(msg_type_) << endl;
			// else
			// {
			// 	cout << "HostName is: " << Hname << endl;
			// }
		}
		break;
	}
	case 5:
	{
		// size_t list_len;
		// int fd;
		// sockaddr_in addr;
		// char addr_str[INET_ADDRSTRLEN];
		msg_type = static_cast<unsigned>(MsgType::kList);
		if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
			cout << "Communication Error! " << endl;
		else
		{
			// recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
			// // recv(sockfd, reinterpret_cast<void *>(&list_len), sizeof(list_len), 0);
			// const MsgType msg_type_ = static_cast<MsgType>(msg_type);
			// if (msg_type_ != MsgType::kList)
			// 	cout << "Recieve Type Error! Code: " << static_cast<int>(msg_type_) << endl;
			// else
			// {
			// 	// for (size_t i = 0; i < list_len; i++, cout << endl)
			// 	// {
			// 	// 	recv(sockfd, reinterpret_cast<void *>(&fd), sizeof(fd), 0);
			// 	// 	recv(sockfd, reinterpret_cast<void *>(&addr), sizeof(addr), 0);
			// 	// 	inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
			// 	// 	cout << "Socket fd " << fd << " Host Address " << addr_str << " : " << addr.sin_port;
			// 	// }
			// }
		}
		break;
	}

	case 6:
	{
		int dst;
		string msg;
		size_t msg_len;
		msg_type = static_cast<unsigned>(MsgType::kMsg);
		if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
			cout << "Communication Error! " << endl;
		else
		{
			cout << "please enter your destination host:" << endl;
			cin >> dst;
			if (send(sockfd, reinterpret_cast<void *>(&dst), sizeof(dst), 0) == -1)
				cout << "Communication Error! " << endl;
			else
			{
				cout << "please enter your message to host " << dst << " :" << endl;
				getline(cin, msg);
				getline(cin, msg);
				msg_len = msg.length() * sizeof(char);
				if (send(sockfd, reinterpret_cast<void *>(&msg_len), sizeof(msg_len), 0) == -1)
					cout << "Communication Error! " << endl;
				else if (send(sockfd, reinterpret_cast<void *>(msg.data()), msg_len, 0) == -1)
					cout << "Communication Error! " << endl;
				else
				{
					recv(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0);
					const MsgType msg_type_ = static_cast<MsgType>(msg_type);
					if (msg_type_ == MsgType::kSuccess)
						cout << "Message Sent Success!" << endl;
					else
						cout << "Message Sent Error!" << endl;
					cin.sync();
				}
			}
		}
		break;
	}

	case 7:
	{
		// heart_beating = false;
		cout << "Bye bye~" << endl;
		quit = true;
		return -1;
	}

	default:
		cout << "Wrong Option Number!" << endl;
		getline(cin, temp);
		break;
	}
	return 0;
}
