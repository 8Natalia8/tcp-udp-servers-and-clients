#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <windows.h>
#include <winsock2.h>
#include<ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct msg {
	unsigned int num;
	string date;
	unsigned short year;
	short int AA;
	string phone;
	string message;
};

struct msg_protocol {
	unsigned int num;
	unsigned char dd;
	unsigned char mm;
	unsigned short year;
	short int AA;
	string phone;
	string message;
};

int set_non_block_mode(int s)
{
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
}
int init()
{
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
}
void deinit() {
	WSACleanup();
}
int sock_err(const char* function, int s)
{
	int err;
	err = WSAGetLastError();
	cout << function << ": socket error: " << err << endl;
	return -1;
}
int send_msg(int cli_socket)//отправляем клиенту
{
	char buffer[2] = { 'o', 'k' };
	int ret = send(cli_socket, buffer, 2, 0);
	if (ret <= 0) {
		return sock_err("send", cli_socket);
	}
	return 0;
}
int arg_err(const char* what) {
	cout << "Invalid arguments: " << what << endl;
	return -1;
}

int file_err(const char* what) {
	cout << "Problem with file: " << what << endl;
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
struct cli {
	sockaddr_in addr;//ip address
	int cs;//типа сокет
	vector<msg_protocol> msgs;
	int sent_oks;
};
string to_string_my(unsigned char c) {
	string str;
	str.reserve(2);//?
	if (c<10) {
		str.push_back('0');
		str.push_back(c + '0');
	}
	else {
		str = to_string(c);
	}
	return str;
}
int print_msg(cli client, msg_protocol msg_protocol) {
	unsigned int netip = client.addr.sin_addr.s_addr;
	unsigned int netport = client.addr.sin_port;
	string dd = to_string_my(msg_protocol.dd);
	string mm = to_string_my(msg_protocol.mm);

	string phone=msg_protocol.phone;
	string message=msg_protocol.message;
	uint16_t port = htons(netport);

ofstream out;
out.open("msg.txt", ios_base::out | ios_base::app);
out<<(netip&0xff)<<"." << (netip >> 8 & 0xff) << "." << (netip >> 16 & 0xff) << "." << (netip >> 24 & 0xff) << ":" << port << " ";
out << dd << "." << mm << "." << msg_protocol.year << " " << msg_protocol.AA << "" << phone << " "<<message << endl;
out.close();
if (message == "stop") {
	return 1;
}
return 0;
}
int main(int argc, char * argv[])
{
	int port = 9000;//только 1 порт прослушиваем
	int flags = 0;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	else
	{
		cout << "TRY MORE" << endl;
	}
	init();
	vector<cli> clients;//клиентские соединения
	// Создание TCP-сокета
	int listen_socket = s_open();
	set_non_block_mode(listen_socket);
	struct sockaddr_in listen_addr;
	memset(&listen_addr, 0, sizeof(sockaddr_in));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);// Все адреса
	// Связывание сокета и адреса прослушивания
	if (bind(listen_socket, (struct sockaddr*) &listen_addr, sizeof(listen_addr)) < 0) {
		return sock_err("bind", listen_socket);
	}
	// Начало прослушивания
	if (listen(listen_socket, 1) < 0) {
		return sock_err("listen", listen_socket);
	}
	cout << "LISTENING ON PORT: " << argv[1] << endl;
	struct timeval tv = { 0, 100 * 1000 };
	
	WSAEVENT events[2];
	int cnt=0;
	int z = 0;
	bool time_to_stop = 0;
	while (!time_to_stop) 
	{
		cnt++;
		events[0] = WSACreateEvent();
		events[1] = WSACreateEvent();
		WSAEventSelect(listen_socket, events[0], FD_ACCEPT);//сопоставили событие с сокетом
		for (int i = 0; i < z; i++) {
			WSAEventSelect(clients[i].cs, events[1], FD_READ);
		}
		WSANETWORKEVENTS ne;
		DWORD dw = WSAWaitForMultipleEvents(2, events, false, 1000, false);
		WSAResetEvent(events[0]);
		WSAResetEvent(events[1]);
		if (0 == WSAEnumNetworkEvents(listen_socket, events[0], &ne) && (ne.lNetworkEvents & FD_ACCEPT)) {
			//поступило событие, принимаем подключение
			cli cl;
			memset(&cl, 0, sizeof(cli));
			int addrlen = sizeof(cl.addr);
			cl.cs = accept(listen_socket, (struct sockaddr*) &cl.addr, (socklen_t *)&addrlen);
			if (cl.cs < 0) {
				return sock_err("accept", cl.cs);
			}
			//мб тут циклом сделать?

			cout << "NEW CLIENT ACCEPTED!" << endl;
			cout << "Client connected: " << (cl.addr.sin_addr.s_addr & 0xff) << "."
				<< (cl.addr.sin_addr.s_addr >> 8 & 0xff) << "."
				<< (cl.addr.sin_addr.s_addr >> 16 & 0xff) << "."
				<< (cl.addr.sin_addr.s_addr >> 24 & 0xff) << ":"
				<< ntohs(cl.addr.sin_port) << endl;
			clients.push_back(cl);

			z++;
		}
			for (int i = 0; i < z; i++) {
				if (0 == WSAEnumNetworkEvents(clients[i].cs, events[1], &ne)) 
				{
					if (ne.lNetworkEvents & FD_READ) {
						int rcv = 0;
						if (clients[i].msgs.size() == 0) {
							char modebuff[3];//принимаем пут
							memset(&modebuff, 0, sizeof(modebuff));
							rcv = recv(clients[i].cs, modebuff, sizeof(modebuff), 0); //надо принимать по одному байту
							while (rcv < 3) {
								rcv += recv(clients[i].cs, modebuff, sizeof(modebuff), 0);
							}
							//cout << modebuff << endl;
						}
						char buff[4096];
						memset(&buff, 0, sizeof(buff));
						msg_protocol msg;
						rcv = 0;
						rcv += (int)recv(clients[i].cs, buff + rcv, sizeof(buff) - rcv, 0);
						if (rcv == 0) { //клиент отключился

							cout << "Client disconected: " << (clients[i].addr.sin_addr.s_addr & 0xff) << "."
								<< (clients[i].addr.sin_addr.s_addr >> 8 & 0xff) << "."
								<< (clients[i].addr.sin_addr.s_addr >> 16 & 0xff) << "."
								<< (clients[i].addr.sin_addr.s_addr >> 24 & 0xff) << ":"
								<< htons(clients[i].addr.sin_port) << endl;
							clients.erase(clients.begin() + i);
							i = 0;
							continue;
						}
						//cout << "1" << endl;
						//КАКАЯ ТО ЗАЛУПА
					while (buff[rcv - 1] != '\0' || rcv < 20) {
							if (rcv == 4096) {
								break;
							}
							rcv += (int)recv(clients[i].cs, buff + rcv, sizeof(buff) - rcv, 0);
							cout << "2" << endl;

					}
					rcv += (int)recv(clients[i].cs, buff + rcv, sizeof(buff) - rcv, 0);
					cout << "got " << rcv << " bytes " << endl;
						unsigned int  num = buff[0] * 16777216 + buff[1] * 65536 + buff[2] * 256 + buff[3];
						msg.num = num;
						if (num == clients[i].msgs.size()) {//clients[i].size
							msg.dd = buff[4];
							cout << msg.dd << endl;
							msg.mm = buff[5];
							cout << msg.mm << endl;

							unsigned short year = (unsigned short)((unsigned char)buff[6] * 256 + (unsigned char)buff[7]);
							msg.year = year;
							short int AA = (short int)((unsigned char)buff[8] * 256 + (unsigned char)buff[9]);//unsigned т.к. переполняется 
							msg.AA = AA;
							clients[i].msgs.push_back(msg);
							for (int j =10; j<22; j++) {
								clients[i].msgs[num].message.push_back(buff[j]);
							}
							clients[i].msgs[num].message.push_back(' ');
							//clients[i].msgs[num].message.resize(clients[i].msgs[num].message.size - 1);
							for (int j = 22; buff[j]!='\0'; j++) {
								clients[i].msgs[num].message.push_back(buff[j]);
							}
						}
						else {
							if (clients[i].msgs.size() != 0) {
								unsigned int pos = clients[i].msgs.size() - 1;
								for (int j = 0; buff[j] != '\0'; j++) {
									clients[i].msgs[pos].message.push_back(buff[j]);
								}
							}
						}
						memset(&buff, 0, sizeof(buff));
						while (rcv == 4096) {
							unsigned int pos = clients[i].msgs.size() - 1;
							rcv = recv(clients[i].cs, buff, sizeof(buff), 0);
							for (int j = 0; buff[j] != '\0'; j++) {
								clients[i].msgs[pos].message.push_back(buff[j]);
							}
							memset(&buff, 0, sizeof(buff));
						}
						time_to_stop = print_msg(clients[i], clients[i].msgs[clients[i].msgs.size() - 1]);
						if (time_to_stop) {
							cout << " stop msg arrived." << endl;
							time_to_stop = 1;
						}
					}
						int msg_num = clients[i].msgs.size();
						for (; clients[i].sent_oks < msg_num; clients[i].sent_oks++) {
							char buff[2];
							buff[0] = 'o';
							buff[1] = 'k';
							int sent = 0;
							int flags = 0;
							int ret = 0;

							while (sent < 2) {
								ret = send(clients[i].cs, buff + sent, 2 - sent, flags);
								if (ret <= 0) {
									return sock_err("send", clients[i].cs);
								}
								sent += ret;
							}
							memset(&buff, 0, sizeof(char) * 2);
						}
						if (time_to_stop) {
							for (int i = 0; i <clients[i].msgs.size(); i++) {
								s_close(clients[i].cs);
							}
							cout << "DELETING SOCKET AND CLOSING THE CONNECTION" << endl;
						}
				}
			}
		}

	s_close(listen_socket); 
	deinit();
	return 0;

}