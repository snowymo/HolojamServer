#include <string>
using std::string;
const unsigned int MyServersDataPort = 1511;
const unsigned int MyServersCommandPort = 1510;
const unsigned int PORT = 1611;
const string MULTICAST_IP = "224.1.1.1";

class BindIP {
public:
	// IP for this host computer
	static string MULTICAST_BIND_IP;
	// IP for UNICAST BIND IP
	static string UNICAST_BIND_IP;
};