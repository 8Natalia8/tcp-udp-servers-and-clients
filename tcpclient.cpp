#include <iostream>
#include<sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#include<string>
#include <string.h>
#include <malloc.h>
#include<stdlib.h>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>

using namespace std;

struct msg {
	unsigned int num;
	string date;
	unsigned short year;//??
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

int init() {
	return 1;
}
void deinit() {

}
int sock_err(const char* function, int s) {
	int err = errno;
	printf("%s: socket error: %d\n", function, err);
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
	close(s);
}
int arg_err(const char* smth) {
	printf("Invalid arguments:%c\n ", smth);
	return -1;
}
int file_err(const char* what) {
	cout << "Problem with file: " << what << endl;
	return -1;
}
int parse(char* str, char* ip, char* port) {
	string sock(str);
	char needle = ':';
	size_t found = sock.find(needle);

	if (found > 15) {   //255.255.255.255
		return arg_err("ip error");
	}

	if (sock.length() > found + 16) {
		return arg_err("too big port");
	}

	if (found == string::npos) {
		return arg_err("not found ':'");
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
	protocol.phone= msg.phone;
	//pos2 = pos1 + 1;
//cout << pos1<<" "<<pos2<<" "<<endl;
//cout << protocol.pn0<< " "<< endl;

	protocol.message = msg.message;
//cout << protocol.pn0 << " "<< protocol.pn1<< " " <<endl;
	return protocol;
}
int send_msg(int socket, struct msg m) {
	struct msg_protocol msg = parse_msg_protocol(m);
	char c;
	int res;
	const char num[4] = {
		(msg.num >> 24) & 0xff,
		(msg.num >> 16) & 0xff,
		(msg.num >> 8) & 0xff,
		msg.num & 0xff
	};
	const char year[2] = {
		//(msg.year >> 24) & 0xff,
		//(msg.year >> 16) & 0xff,
		(msg.year >> 8) & 0xff,
		msg.year & 0xff
	};
	const char AA[4] = {
		//(msg.AA >> 24) & 0xff,
		//(msg.AA >> 16) & 0xff,
		(msg.AA >> 8) & 0xff,
		msg.AA & 0xff
	};
	int len = msg.message.length();
	char* message = new char[len + 1];
	memcpy(message, msg.message.c_str(), len);
	message[len] = 0;
	const char* cmsg = message;
char*phone_number=new char[12];
memcpy(phone_number,msg.phone.c_str(),12);
//phone_number[13]=0;
const char* cphone=phone_number;
	res = send(socket, num, sizeof(char) * 4, MSG_NOSIGNAL);
	if (res < 0) {
		return sock_err("send", socket);
	}
	c = msg.dd;
	res = send(socket, &c, sizeof(char), MSG_NOSIGNAL);
	if (res < 0) {
		return sock_err("send", socket);
	}
	c = msg.mm;
	res = send(socket, &c, sizeof(char), MSG_NOSIGNAL);
	if (res < 0) {
		return sock_err("send", socket);
	}
	res = send(socket, year, sizeof(char) * 2, MSG_NOSIGNAL);
	if (res < 0) {
		return sock_err("send", socket);
	}
	res = send(socket, AA, sizeof(char) * 2, MSG_NOSIGNAL);
	if (res < 0) {
		return sock_err("send", socket);
	}
	res=send(socket,cphone,12,MSG_NOSIGNAL);
if (res < 0) {
		return sock_err("send", socket);
	}
	res = send(socket, cmsg, len + 1, MSG_NOSIGNAL);
	if (res < 0) {
		return sock_err("send", socket);
	}
delete phone_number;
	delete message;
	return 0;
}
int recv_response(int socket) {
	char buff[2];
	int res = 0;
	int tryn = 0;

	while (res == 0) {
		res = recv(socket, buff, sizeof(buff), 0);
		tryn++;
		if (tryn == 20) {
			cout << "Connection closed by server " << endl;
			return -1;
		}
	}
	if (res == 2) {
		cout << "Got OK" << endl;
		return 0;
	}
	if (res == 1) {
		cout << "Got O";
		res = 0;
		while (res == 0) {
			res = recv(socket, buff, sizeof(buff), 0);
		}
		if (res == 1) {
			cout << "K" << endl;
			return 0;
		}
	}
	return 0;
}
int send_mode(int socket) {
	int res = send(socket, "put", sizeof(char) * 3, 0);
	if (res < 0) {
		return sock_err("Send_mode", res);
	}
	return 0;
}

int main(int argc, char **argv) {
	init();
	if (argc != 3) {
		return arg_err("FEW ARGUMENTAS. Try again");
	}
	char* ip = (char*)calloc(16, sizeof(char));
	char* port = (char*)calloc(17, sizeof(char));
	int parse_result = parse(argv[1], *(&ip), *(&port));
	if (parse_result < 0) {
		return -1;
	}
	int res = 1;
	int connection_try = 0;
	struct sockaddr_in addr;
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return sock_err("socket", s);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));
	addr.sin_addr.s_addr = inet_addr(ip);
	while (connection_try++ < 10) {
		printf("try %d ... ", connection_try);
		res = connect(s, (struct sockaddr*) &addr, sizeof(addr));
		if (res == 0) {
			printf("Good\n");
			if (send_mode(s) < 0) {
				return sock_err("problem with sending", s);
			}
			break;
		}
		else {
			printf("Bad\n");
		}
		usleep(100 * 1000);
	}
	if (res != 0) {
		return sock_err("Connection", s);
	}
	ifstream input;
	input.open(argv[2]);
	if (!input.is_open()) {
		return file_err("open file");
	}
	string line;
	int numline = 0;
	while (getline(input, line)) {
		int snd = 0;
		if (line.size() < 20) { 
			continue;
		}
		struct msg msg = parse_line(line, numline);
		if (snd = send_msg(s, msg) != 0) {
			return snd;
		}
		numline++;
	}
	input.close();
	while (numline--) {
		if (recv_response(s) < 0) {
			break;
		}
	}
	s_close(s);
	deinit();

}
