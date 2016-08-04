#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Stream.h"

using std::string;
using std::vector;

const unsigned int MyServersDataPort = 1511;
const unsigned int MyServersCommandPort = 1510;
const unsigned int PORT = 1611;
const string MULTICAST_IP = "224.1.1.1";

class BindIP {
public:
	Stream* multicast_stream;
	vector<Stream*> unicast_streams;
	// IP for this host computer
	static string MULTICAST_BIND_IP;
	// IP for UNICAST BIND IP
	static string UNICAST_BIND_IP;
	// Have IPs been set?
	static bool IPsSet;
	// Set IP method
	static void setIPAddressBinds();
	BindIP();
	//virtual ~BindIP();
};