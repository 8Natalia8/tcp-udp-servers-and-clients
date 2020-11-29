#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include<fstream>
//#include<unistd.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

void err_exit(FILE *F, int S)
{
	closesocket(S);
	fclose(F);
	printf("Invalid data\n");
}
int init() {
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
}
void deinit() {
	WSACleanup();
}
int sock_err(const char *function, int s)
{
	int err;
	err = WSAGetLastError();
	cout << function << ": socket error" << err << endl; 
	return -1;
}
int arg_err(const char*what) {
	cout << "INVALID ARGS: " << what << endl;
	return -1;
}
int file_err(const char *what) {
	cout << "PROBLEM WITH FILE: " << what << endl;
	return -1;
}
int s_open() {
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		return sock_err("socket", s);
	}
	else {
		return s;
	}
}
void s_close(int s) {
	closesocket(s);
}
struct msg {
	unsigned int num;
	string date;
	unsigned short year;
	short int AA;
	string phone;
	string message;
};
struct msg_protocol{
	unsigned int num;
	unsigned char dd;
	unsigned char mm;
	unsigned short year;
	short int AA;
	string phone;
	string message;
};
struct send_buf {
	unsigned char num[4];
	unsigned char dd;
	unsigned char mm;
	unsigned char year[2];
	unsigned char AA[2];
	const char*phone;
	const char*message;
};
int parse(char *str, char *ip, char*port) {
	string sock(str);
	char needle = ':';
	size_t found = sock.find(needle);
	if (found > 15) {
		return arg_err("ip error");
	}
	if (sock.length() > found + 16) {
		return arg_err("too big port");
	}
	if (found == string::npos) {
		return arg_err("not found ':' ");
	}
	strncpy(*(&ip), sock.c_str(), found);
	strncpy(*(&port), sock.c_str() + (int)found + 1, sock.length() - found);
	return 0;
}
struct msg parse_line(string line, int numline) {
	struct msg msg;
	size_t pos1;
	size_t pos2;
	msg.num = numline;
	pos1 = line.find(" ", 0);
	msg.date = line.substr(0, 6);
	msg.year = atoi(line.substr(6, 4).c_str());

	pos2 = pos1 + 1;
	pos1 = line.find(" ", pos2);
	msg.AA = atoi(line.substr(pos2, pos1 - pos2).c_str());//atoi???

	pos2 = pos1 + 1;
	pos1 = line.find(" ", pos2);
	msg.phone = line.substr(pos2, pos1 - pos2);
	pos2 = pos1 + 1;
	msg.message = line.substr(pos2);
	//cout << msg.date << " " << msg.year << " " <<  msg.AA << " "<<  msg.phone << " " <<msg.message << endl;
	return msg;
}
//год ок
struct msg_protocol parse_msg_protocol(struct msg msg) {
	struct msg_protocol protocol;
	size_t pos1 = 0;
	size_t pos2 = 0;
	protocol.num = msg.num;
	pos1 = msg.date.find(".", 0);
	protocol.dd = atoi(msg.date.substr(pos2, pos1 - pos2).c_str());
	//cout<<protocol.dd<<" "<<endl;
	pos2 = pos1 + 1;
	//cout << pos2<<" "<<endl;
	pos1 = msg.date.find(".", pos2);//5 symbol
									//cout << pos1<<" "<<endl;
	protocol.mm = atoi(msg.date.substr(pos2, pos1 - pos2).c_str());
	protocol.year = msg.year;;
	protocol.AA = msg.AA;//ok
						 //cout <<msg.phone<<" "<<endl;//Здесь все норм с телефоном
	pos1 = 0;
	pos2 = 0;
	//pos1 = msg.phone.find("+",pos1);
	protocol.phone = msg.phone;
	//pos2 = pos1 + 1;
	//cout << pos1<<" "<<pos2<<" "<<endl;
	//cout << protocol.pn0<< " "<< endl;

	protocol.message = msg.message;
	//cout << protocol.pn0 << " "<< protocol.pn1<< " " <<endl;
	return protocol;
}
int send_msg(int s, msg m, struct sockaddr_in *addr) {
	struct msg_protocol msg = parse_msg_protocol(m);
	char c;
	int res;
	const unsigned char num[4] = {
		(const unsigned char)((msg.num >> 24) & 0xff),
		(const unsigned char)((msg.num >> 16) & 0xff),
		(const unsigned char)((msg.num >> 8) & 0xff),
		(const unsigned char)(msg.num & 0xff)
	};
	const unsigned char year[2] = {
		(const unsigned char)((msg.year >> 8) & 0xff),
		(const unsigned char)(msg.year & 0xff)
	};
	const unsigned char AA[2] = {
		(const unsigned char)((msg.AA >> 8) & 0xff),
		(const unsigned char)(msg.AA & 0xff)
	};
	char*phone_number = new char[12];
	memcpy(phone_number, msg.phone.c_str(), 12);
	//const char* cphone = phone_number;
	int msglen =(int) msg.message.length();
	char* message = new char[msglen + 1];
	memcpy(message, msg.message.c_str(), (size_t)msglen);
	message[msglen] = 0;
	unsigned char*to_send = new unsigned char[10+12 +msglen + 1];
	to_send[0] = num[0];
	to_send[1] = num[1];
	to_send[2] = num[2];
	to_send[3] = num[3];
	to_send[4] = msg.dd;
	to_send[5] = msg.mm;
	to_send[6] = year[0];
	to_send[7] = year[1];
	to_send[8] = AA[0];
	to_send[9] = AA[1];
	for (int j = 0; j < 12; j++) {
		to_send[j + 10] = (unsigned char)phone_number[j];
	}
	int i;
	for (i = 0; i < msglen; i++) {
		to_send[i + 10 + 12 ] = (unsigned char)message[i];
	}
	to_send[i + 10+ 12] = '\0';
	res = (int)sendto(s, (char*)to_send, sizeof(char)*(10 + 12) + sizeof(char)*(msglen + 1), 0, (struct sockaddr*) addr, sizeof(struct sockaddr_in));
	//посмотреть преобразование ансайн в чар
	if (res <= 0) {
		return sock_err("sendto", s);
	}
	delete phone_number;
	delete message;
	delete to_send;
	return 0;
}
int main(int argc, char *argv[]) {
	if (argc != 3) {
		return arg_err("too few arguments. Need ip:port and 1.txt");
	}
	char* ip = (char*)calloc(16, sizeof(char));
	char* port = (char*)calloc(17, sizeof(char));
	int parse_result = parse(argv[1], *(&ip), *(&port));
	if (parse_result < 0) {
		return -1;
	}
	int res = 0;
	int connection_try = 0;
	struct sockaddr_in addr;
	init();
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		return sock_err("socket", s);
	}
	int flags = 0;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	addr.sin_addr.s_addr = inet_addr(ip);

	ifstream in;
	in.open(argv[2]);
	if (!in.is_open()) {
		return file_err("problems with opening");
	}
	vector <msg> msgs;
		string curline;
	int numline = 0;
	while (getline(in, curline)) {
		if (curline.size() < 20) { //если вдруг есть пустые или неполные строки
			continue;
		}
		struct msg msg = parse_line(curline, numline);
		msgs.push_back(msg);//phone_number?
		numline++;
	}
	in.close();
	int sent_msgs = 0;
	while (sent_msgs<20 && msgs.size() != 0) {
		cout << msgs.size() << " msgs left in queue" << endl;
		for (int i = 0; i < min((int)msgs.size(), 20); i++) {
			int sent = send_msg(s, msgs[i], &addr);
			if (sent < 0) {
				return sent;
			}
		}
		unsigned char response[4 * 20 * sizeof(char)];
		unsigned int nums[20];
		memset(&response, 0, sizeof(unsigned char) * 4 * 20);
		memset(&nums, 0, sizeof(unsigned int) * 20);
		struct timeval tv = { 0, 100 * 1000 };
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(s, &fds);
		res = select(s + 1, &fds, 0, 0, &tv);
		if (res > 0) {
			struct sockaddr_in resaddr;
			socklen_t addrlen = sizeof(resaddr);
			int received = (int)recvfrom(s, (char*)response, sizeof(response), 0, (struct sockaddr *) &resaddr, &addrlen);
			if (received <= 0) {
				return sock_err("recvfrom", s);
			}
			for (int i = 0; i < received; i += 4) {
				nums[i / 4] =
					(unsigned int)(response[i] * 16777216 + response[i + 1] * 65536 + response[i + 2] * 256 +
						response[i + 3]);
			}
			for (int j = 0; j < received / 4; j++) {
				for (vector<msg>::iterator it = msgs.begin(); it != msgs.end(); it++) {
					if (it->num == nums[j]) {
						msgs.erase(it);
						sent_msgs++;
						break;
					}
				}
			}
		}
		cout << msgs.size() << " msgs left in queue" << endl;
	}

	return 0;
}
