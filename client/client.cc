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
#include <signal.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include "../MsgDef.hpp"
#include "Queue.hpp"

using namespace std;

extern int errno;
int sockfd;
std::atomic<bool> conn;
std::atomic<bool> quit;
int GetOption();
int Request(int option, std::mutex *mutex);
void RecvMsg();
#ifdef HEARTBEAT
std::atomic<bool> recv_heartbeat;
auto Heartbeat = [](int *fd, std::atomic<bool> *conn, std::atomic<bool> *quit, std::atomic<bool> *recv_heartbeat, std::mutex *mutex)
{
	signal(SIGPIPE, SIG_IGN);  // don't exit when send's errno = 32
	int heartbeat_counter = 0;
	unsigned msg_type = static_cast<unsigned>(MsgType::kHeartBeat);
	while (!*quit)
	{
		std::this_thread::sleep_for(kHeartBeatInterval);
		if (*conn) {
			{
				std::lock_guard<std::mutex> lock(*mutex);
				if (!~send(*fd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0)) {
					if (errno == 104 || errno == 9 || errno == 32)
						return;  // peer closed || closed by server
					std::cerr << "send() heartbeat failed! errno: " << errno
										<< std::endl;
				}
			}
			if (*recv_heartbeat) {
				heartbeat_counter = 0;
				*recv_heartbeat = false;
			} else {
				heartbeat_counter++;
				if (heartbeat_counter >= kHeartBeatThreshold) return;
			}
		} else {
			heartbeat_counter = 0;
		}
	}
};
#endif

ThreadSafeQueue<string> msgQ;

int main(int argc, char *argv[])
{
	string msg;
	int option;
	quit = false;
	conn = false;
	std::mutex mutex;
	future<void> recvMsg = async(launch::async, RecvMsg);
	future<int> getOption = async(launch::async, GetOption);

#ifdef HEARTBEAT
	std::future<void> alive = std::async(std::launch::async, Heartbeat, &sockfd, &conn, &quit, &recv_heartbeat, &mutex);
#endif

	cout << "please enter service number: \n-1 Connect Server\n-2 Close Connection\n-3 Get Time\n-4 Get HostName\n-5 Get List\n-6 Send Message\n-7 Quit\n";
	while (1)
	{
		if (!msgQ.empty())
		{
			msgQ.pop(msg);
			cout << msg << endl;
		}
		if (getOption.wait_for(chrono::milliseconds(1)) == std::future_status::ready)
		{
			option = getOption.get();
			if (Request(option, &mutex) == -1)
				return 0;
			cout << "please enter service number: \n-1 Connect Server\n-2 Close Connection\n-3 Get Time\n-4 Get HostName\n-5 Get List\n-6 Send Message\n-7 Quit\n";
			getOption = async(launch::async, GetOption);
		}
#ifdef HEARTBEAT
		if (conn && alive.wait_for(std::chrono::milliseconds(1)) ==
				std::future_status::ready)
		{
			cout << "Communication Error, close locally." << endl;
			close(sockfd);
			conn = false;
			alive = std::async(std::launch::async, Heartbeat, &sockfd, &conn, &quit, &recv_heartbeat, &mutex);
		}
#endif
	}
}

int GetOption()
{
	int option;
	cin >> option;
	return option;
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
				break;
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
				break;
			}
			case MsgType::kSuccess:
				msgQ.push("Message Sent Success!");
				break;

			case MsgType::kError:
				msgQ.push("Message Sent Error!");
				break;
#ifdef HEARTBEAT
			case MsgType::kHeartBeat:
				recv_heartbeat = true;
				break;
#endif
			default:
				break;
			}
		}
	}
}

int Request(int option, std::mutex *mutex)
{
	// int option;
	unsigned msg_type;
	string ipaddr;
	static int port;
	struct sockaddr_in serverAddr;
	string temp;

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
		//
		if(!conn)
		{
			cout << "No connection on! Please connect to server first" << endl;
			break;
		}
		msg_type = static_cast<unsigned>(MsgType::kDisconnect);
		{
			std::lock_guard<std::mutex> lock(*mutex);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error, close locally." << endl;
		}
		close(sockfd);
		cout << "Connection to " << ipaddr << ":" << port << " is closed! \n"
			 << endl;
		conn = false;
		break;
	}
	case 3:
	{
		//获取时间

		msg_type = static_cast<unsigned>(MsgType::kTime);
		{
			std::lock_guard<std::mutex> lock(*mutex);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
		}

		break;
	}
	case 4:
	{

		msg_type = static_cast<unsigned>(MsgType::kHostname);
		{
			std::lock_guard<std::mutex> lock(*mutex);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
		}

		break;
	}
	case 5:
	{

		msg_type = static_cast<unsigned>(MsgType::kList);
		{
			std::lock_guard<std::mutex> lock(*mutex);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
		}
		break;
	}

	case 6:
	{
		int dst;
		string msg;
		size_t msg_len;
		msg_type = static_cast<unsigned>(MsgType::kMsg);
		if(!conn)
		{
			cout << "No connection on! Please connect to server first" << endl;
			break;
		}
		cout << "please enter your destination host:" << endl;
		cin >> dst;
		cout << "please enter your message to host " << dst << " :" << endl;
		getline(cin, msg);
		getline(cin, msg);
		msg_len = msg.length() * sizeof(char);
		{
			std::lock_guard<std::mutex> lock(*mutex);
			if (send(sockfd, reinterpret_cast<void *>(&msg_type), sizeof(msg_type), 0) == -1)
				cout << "Communication Error! " << endl;
			else
			{
				if (send(sockfd, reinterpret_cast<void *>(&dst), sizeof(dst), 0) == -1)
					cout << "Communication Error! " << endl;
				else
				{
					if (send(sockfd, reinterpret_cast<void *>(&msg_len), sizeof(msg_len), 0) == -1)
						cout << "Communication Error! " << endl;
					else if (send(sockfd, reinterpret_cast<void *>(msg.data()), msg_len, 0) == -1)
						cout << "Communication Error! " << endl;
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
