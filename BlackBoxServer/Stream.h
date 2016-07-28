#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#define LCL_BROADCAST
//#define RMT_BROADCAST // TODO: uncomment
//#define RMT_RCV // TODO: comment

using std::string;

// IP for this host computer
#ifdef LCL_BROADCAST
static const string IP_ADDR = "192.168.1.44";
#elif defined RMT_BROADCAST
static const string IP_ADDR = ""; // TODO: Enter MAGNET computer IP
#elif defined RMT_RCV
static const string IP_ADDR = "128.122.47.161";
#endif

class Stream {
	SOCKET s;
	struct sockaddr_in addr;
	struct sockaddr_in bind_addr;
	void handleError(int err/*, string message */);
public:
	Stream(PCSTR ip, int server_port, bool multicast);
	void send(char* packet, int length);
	~Stream();
};