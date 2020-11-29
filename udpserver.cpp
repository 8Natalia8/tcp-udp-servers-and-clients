#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <vector>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>

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
struct cli {
	sockaddr_in addr;
	vector<unsigned int> msgnums;
	vector<msg_protocol> msgs;
};
int init() {
	return 1;
}

void deinit() {
}

int sock_err(const char* function, int s) {
	int err=errno;
	cout << function << ": socket error: " << err << endl;
	return -1;
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
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		return sock_err("socket", s);
	}
	else {
		return s;
	}
}

void s_close(int s) {
	close(s);
}
vector<cli>::iterator find_cli(vector<cli> *clients, sockaddr_in addr) {
	for (vector<cli>::iterator it = clients->begin(); it != clients->end(); ++it) {
		if (!memcmp(&it->addr, &addr, sizeof(sockaddr))) {
			return it;
		}
	}
	return clients->end();
}
void to_date(char buff[3], unsigned char c) {
	if (c >= 50) {
		buff[0] = '5';
	}
	else if (c >= 40) {
		buff[0] = '4';
	}
	else if (c >= 30) {
		buff[0] = '3';
	}
	else if (c >= 20) {
		buff[0] = '2';
	}
	else if (c >= 10) {
		buff[0] = '1';
	}
	else {
		buff[0] = '0';
	}
	buff[1] = c % 10 + 48;
	buff[2] = 0;
}

int print_msg(vector<cli>::iterator it_client, msg_protocol msg_protocol) {
	unsigned int netip = it_client->addr.sin_addr.s_addr;
	unsigned int netport = it_client->addr.sin_port;
	char dd[3];
	to_date(dd, msg_protocol.dd);
	char mm[3];
	to_date(mm, msg_protocol.mm);
	string phone(msg_protocol.phone);
	string message(msg_protocol.message);
//unsigned short year = htons(msg_protocol.year);//!!!!!
	//short int AA = htons(msg_protocol.AA);
	unsigned int port = htons(netport);
	ofstream out;
	out.open("msg.txt", ios_base::out | ios_base::app);
	out << (netip & 0xff) << "." << (netip >> 8 & 0xff) << "." << (netip >> 16 & 0xff) << "." << (netip >> 24 & 0xff) << ":" << port << " ";
	out << dd << "." << mm << "." <<msg_protocol.year << " " << msg_protocol.AA << " " << phone << " " << message << endl;
	out.close();
	if (message == "stop") {
		return 1;
	}
	return 0;
}

unsigned int add_msg(vector<cli>::iterator it_client, char* msg, int msglen) {
	msg_protocol msg_protocol;
	cli client;
	char lenum[4];
	unsigned int msgnum;
	unsigned short year;
	short int AA;
	msgnum = (unsigned int)(msg[0] & 0xff) * 16777216 + (unsigned int)(msg[1] & 0xff) * 65536 + (unsigned int)(msg[2] & 0xff) * 256 + (unsigned int)(msg[3] & 0xff);
	vector<unsigned int>::iterator vecend = it_client->msgnums.end();
	for (vector<unsigned int>::iterator it_msgnums = it_client->msgnums.begin(); it_msgnums != vecend; ++it_msgnums) {
		if (msgnum == *it_msgnums) {
			return -1;
		}
	}
	it_client->msgnums.push_back(msgnum);
	msg_protocol.dd = msg[4];
	msg_protocol.mm = msg[5];
	year =  (unsigned short)(msg[6]&0xff) * 256+ (unsigned short)(msg[7] & 0xff);
	msg_protocol.year = year;
	AA = (short int)(msg[8] & 0xff) * 256 + (short int)(msg[9] & 0xff) ;
	msg_protocol.AA = AA;
	for (int i = 10; i < 22; i++) {
		msg_protocol.phone.push_back(msg[i]);
	}
	for (int i = 22; i < msglen && msg[i] != '\0'; i++) {
		msg_protocol.message.push_back(msg[i]);
	}
	it_client->msgs.push_back(msg_protocol);
	return print_msg(it_client, msg_protocol);
}
int main(int argc, char*argv[]) {
	int from_port;
	int to_port;
	init();
	if (argc >3) {
		return arg_err("Too many arguments. Need port and port // port");
	}
	if (argc == 3) {
		from_port = atoi(argv[1]);
		to_port = atoi(argv[2]);
	}
	else if (argc == 2) {
		from_port = to_port = atoi(argv[1]);
	}
	else if (argc == 1) {
		from_port = to_port = 9000;
	}
	int range_port = to_port - from_port + 1;
	int flags =0;
	int* listen_sockets = new int[range_port];
	struct sockaddr_in* listen_addresses = new sockaddr_in[range_port];
	for (int i = 0; i<range_port; i++) {
		listen_sockets[i] = s_open();
		if (listen_sockets[i] < 0) {
			for (int j = 0; j < i; j++) {
				s_close(listen_sockets[j]);
			}
			return listen_sockets[i];
		}
		memset(&listen_addresses[i], 0, sizeof(listen_addresses[i]));
		listen_addresses[i].sin_family = AF_INET;
		listen_addresses[i].sin_port = htons(from_port + i);
		listen_addresses[i].sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(listen_sockets[i], (struct sockaddr*) &listen_addresses[i], sizeof(listen_addresses[i]))) {
			return sock_err("bind", listen_sockets[i]);
		}

		cout << "Listening on port: " << from_port + i << endl;
	}
	vector<cli> *clients;
	struct pollfd pfd[range_port];
	for (int i = 0; i<range_port; i++) {
		pfd[i].fd = listen_sockets[i];//слушательные
		pfd[i].events = POLLIN;
	}
	int time_to_stop = 0;
	while (!time_to_stop) {
		int ev_cnt = poll(pfd, sizeof(pfd) / sizeof(pfd[0]), 1000);//размерность range???
		for (int i = 0; i<range_port; i++) {
			if (pfd[i].revents&POLLIN){
				sockaddr_in from;
				int rcv = 0;
				char rcvbuff[65536];
				int fromlen = 65536;
				memset(&from, 0, sizeof(sockaddr));
				rcv = recvfrom(listen_sockets[i], rcvbuff, sizeof(rcvbuff), 0, (sockaddr*)&from, &fromlen);
				if (rcv > 0) {
					vector<cli>::iterator it = find_cli(&clients, from);
					if (it != clients.end()) {
						time_to_stop = add_msg(it, rcvbuff, rcv);
						if (time_to_stop == -1) {
							//cout << "Repeated msg" << endl;
							time_to_stop = 0;
							continue;
						}
					}
						else {
							cli client;
							client.addr = from;
							clients.push_back(client);
							it = clients.end() - 1;
							unsigned int port = htons(from.sin_port);//ntohs!!!???
							cout << "New client: " << (from.sin_addr.s_addr & 0xff) << "." << (from.sin_addr.s_addr >> 8 & 0xff) << "." << (from.sin_addr.s_addr >> 16 & 0xff) << "." << (from.sin_addr.s_addr >> 24 & 0xff) << ":" << port << endl;
							time_to_stop = add_msg(clients.end() - 1, rcvbuff, rcv);
						}
						unsigned int sendbuff[5];
						char cbuff[sizeof(char) * 5 * 4];
						int sendsize = 0;
						memset(&sendbuff, 0, 5 * sizeof(int));
						memset(&cbuff, 0, sizeof(char) * 5 * 4);
						int j = 0;
						sendsize = 0;
						vector<unsigned int>::reverse_iterator vecend = it->msgnums.rend();
						for (vector<unsigned int>::reverse_iterator it_msgnum = it->msgnums.rbegin(); it_msgnum != vecend; ++it_msgnum) {
							*(sendbuff + j) = htonl(*it_msgnum);
							sendsize += 4;
							if (j >= 4) {
								break;
							}
							j++;
						}
						sendsize += 4;
						memcpy(cbuff, sendbuff, sendsize);
						sendto(listen_sockets[i], cbuff, sendsize, 0, (sockaddr*) &(it->addr), sizeof(sockaddr));
						if (it->msgnums.size() >= 20) {

							//cout << "Got 20 msgs from client " << (it->addr.sin_addr.s_addr & 0xff) << "." << (it->addr.sin_addr.s_addr >> 8& 0xff) << "." << (it->addr.sin_addr.s_addr >> 16 & 0xff) << "." << (it->addr.sin_addr.s_addr >> 24 & 0xff) << ":" << htons(it->addr.sin_port) << "   Removing from db" << endl;
							//clients.erase(it);
						}
					}
				}
			}
		}
	

	for (int i = 0; i<range_port; i++) {
		s_close(listen_sockets[i]);
		cout << "End listening on port: " << from_port + i << endl;
	}
	delete[] listen_sockets;
	delete[] listen_addresses;
}

