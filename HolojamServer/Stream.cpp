#include "BindIP.h"
#include <iostream>

using std::cout;
using std::endl;

Stream::Stream(PCSTR ip, int server_port, bool multicast) {

	WSADATA wd;
	WSAStartup(0x02, &wd);
	int err;
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
		cout << "[VR] Could not create socket : " << WSAGetLastError() << endl;;
	}

	int opt_val = 1;
	if (multicast) {
		err = setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&opt_val, sizeof(opt_val));
		handleError(err/*, string */);
	}

	opt_val = 1;
	err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_val, sizeof(opt_val));
	handleError(err/*, string */);

	// Bind to correct NIC
	bind_addr.sin_family = AF_INET;
	string address;
	if (multicast) {
		address = BindIP::MULTICAST_BIND_IP;
	}
	else {
		address = BindIP::UNICAST_BIND_IP;
	}
	err = inet_pton(AF_INET, address.c_str(), &bind_addr.sin_addr); // S_ADDR of our IP for the WiFi interface
	handleError(err/*, string */);

	bind_addr.sin_port = 0;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	err = bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	handleError(err/*, string */);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	// Proper mutlicast group
	err = inet_pton(AF_INET, ip, &addr.sin_addr);
	handleError(err/*, string */);
}
void Stream::send(char* packet, int length) {
	sendto(s, packet, length, 0, (struct sockaddr*) &addr, sizeof(addr));
}

void Stream::handleError(int err/*, string message */) {
	if (err == SOCKET_ERROR) {
		cout << "SOCKET ERROR; HIT TWO KEYS TO ABORT" << endl;
		getchar();
		exit(0);
	}
}

string Stream::getIP() {
	char str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN) != NULL) {
		string s(str);
		return s;
	}
	return "";
}

Stream::~Stream() {
	closesocket(s);
	WSACleanup();
}