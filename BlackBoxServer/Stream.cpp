#include "Stream.h"
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
		if (err == SOCKET_ERROR) {
			cout << "SOCKET ERROR; HIT ANY KEY TO ABORT" << endl;
			getchar();
			exit(0);
		}
	}

	opt_val = 1;
	err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_val, sizeof(opt_val));
	if (err == SOCKET_ERROR) {
		cout << "SOCKET ERROR; HIT ANY KEY TO ABORT" << endl;
		getchar();
		exit(0);
	}

	// Bind to correct NIC
	bind_addr.sin_family = AF_INET;
	err = inet_pton(AF_INET, IP_ADDR.c_str(), &bind_addr.sin_addr); // S_ADDR of our IP for the WiFi interface
	if (err == SOCKET_ERROR) {
		cout << "SOCKET ERROR; HIT ANY KEY TO ABORT" << endl;
		getchar();
		exit(0);
	}
	bind_addr.sin_port = 0;
	err = ::bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	if (err == SOCKET_ERROR) {
		cout << "SOCKET ERROR; HIT ANY KEY TO ABORT" << endl;
		getchar();
		exit(0);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	// Proper mutlicast group
	err = inet_pton(AF_INET, ip, &addr.sin_addr);
	if (err == SOCKET_ERROR) {
		cout << "SOCKET ERROR; HIT ANY KEY TO ABORT" << endl;
		getchar();
		exit(0);
	}
}
void Stream::send(char* packet, int length) {
	sendto(s, packet, length, 0, (struct sockaddr*) &addr, sizeof(addr));
}
Stream::~Stream() {
	closesocket(s);
	WSACleanup();
}