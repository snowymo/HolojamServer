#include "BindIP.h"
#include <string>
#include <iostream>

using std::cout;
using std::endl;
using std::getline;
using std::cin;

string BindIP::MULTICAST_BIND_IP = "192.168.1.44";
string BindIP::UNICAST_BIND_IP = "192.168.1.44";// "128.122.47.161";
bool BindIP::IPsSet = false;

void BindIP::setIPAddressBinds() {
	string tmp;
	cout << "Enter multicast binding IP. Press enter for default (";
	cout << BindIP::MULTICAST_BIND_IP << "):";
	getline(cin, tmp);
	if (tmp != "") {
		BindIP::MULTICAST_BIND_IP = tmp;
	}

	cout << "Enter unicast binding IP. Press enter for default (";
	cout << BindIP::UNICAST_BIND_IP << "):";
	getline(cin, tmp);
	if (tmp != "") {
		BindIP::UNICAST_BIND_IP = tmp;
	}
	IPsSet = true;
}

BindIP::BindIP() {
	multicast_stream = new Stream(MULTICAST_IP.c_str(), PORT, true);
	unicast_streams = vector<Stream*>();
}

BindIP::~BindIP() {
	delete multicast_stream;
	auto it = unicast_streams.begin();
	for (; it != unicast_streams.end();) {
		delete * it;
		it = unicast_streams.erase(it);
	}
}