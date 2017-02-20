/*
BlackBoxServer 4.0
Includes some code from OptiTrack.
*/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <conio.h>
#include "MotiveClient.h"
//#include "update_protocol_v3.pb.h"
#include "BindIP.h"

using std::thread;
using std::cout;
using std::endl;
using std::cin;
using std::getline;

#pragma warning( disable : 4996 )

int PacketServingThread();
//int PacketReceivingThread();
void cleanIPs();
void AddUnicastIP(string ip);

vector<string> ipAddresses;

BindIP* binder;
BindIP* forwardBinder;

int PacketServingThread() {
	while (true) {
		PacketGroup::send(binder->multicast_stream, &binder->unicast_streams);
		Sleep(1);
	}
	return 0;
}

// int PacketReceivingThread() {
// 	WSAData version;        //We need to check the version.
// 	WORD mkword = MAKEWORD(2, 2);
// 	int what = WSAStartup(mkword, &version);
// 	if (what != 0){
// 		cout << "This version is not supported! - \n" << WSAGetLastError() << endl;
// 	}
// 
// 	SOCKET soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// 	
// 	BOOL sockoptval = TRUE;
// 	int sockoptres = setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, (char*)&sockoptval, sizeof(BOOL));
// 	if (sockoptres != 0) {
// 		cout << "Error in Setting Socket Option: " << WSAGetLastError() << endl;
// 	}
// 	if (soc == INVALID_SOCKET)
// 		cout << "Creating socket fail\n";
// 	
// 	sockaddr_in addr;
// 	addr.sin_family = AF_INET;
// 	addr.sin_port = htons(PORT);
// 	addr.sin_addr.s_addr = INADDR_ANY;
// 	addr.sin_addr.s_addr = htonl(INADDR_ANY);
// 
// 	int conn = bind(soc, (sockaddr*)&addr, sizeof(addr));
// 	if (conn == SOCKET_ERROR){
// 		cout << "Error - when connecting " << WSAGetLastError() << endl;
// 		closesocket(soc);
// 		WSACleanup();
// 	}
// 	
// 	int len = 65507;
// 	char *buf = (char*)malloc(sizeof(char) * len);
// 	int flags = 0;
// 
// 	forwardBinder->multicast_stream = new Stream(MULTICAST_IP.c_str(), PORT, true);
// 	
// 	while (true) {
// 		sockaddr_in from_addr;
// 		int from_addr_len = sizeof(from_addr);
// 		int recv_status = recvfrom(soc, buf, len, flags, (sockaddr*)&from_addr, &from_addr_len);
// 		if (recv_status == SOCKET_ERROR) {
// 			cout << "Error in Receiving: " << WSAGetLastError() << endl;
// 		}
// 		//TODO
// 		update_protocol_v3::Update *update = new update_protocol_v3::Update();
// 		//decode
// 		update->ParseFromArray(buf, recv_status);
// 		if (update->label() == "ping") {
// 			char str[INET_ADDRSTRLEN];
// 			if (inet_ntop(AF_INET, &(from_addr.sin_addr), str, INET_ADDRSTRLEN) != NULL) {
// 				string s(str);
// 				AddUnicastIP(str);
// 			}
// 		}
// 
// 		forwardBinder->multicast_stream->send(buf, update->ByteSize());
// 		for (int i = 0; i < forwardBinder->unicast_streams.size(); i++) {
// 			forwardBinder->unicast_streams[i]->send(buf, update->ByteSize());
// 		}
// 		delete(update);
// 		Sleep(1);
// 	}
// 	free(buf);
// }

void AddUnicastIP(string ip) {
	for (int i = 0; i < forwardBinder->unicast_streams.size(); ++i) {
		if (forwardBinder->unicast_streams.at(i)->getIP() == ip) {
			return;
		}
	}
	Stream* stream = new Stream(ip.c_str(), PORT, false);
	forwardBinder->unicast_streams.push_back(stream);
	PacketGroup::AddUnicastIP(ip, &binder->unicast_streams);
}

void initializeIPAddresses() {
	std::ifstream file("ips.txt");
	if (!file.good()) {
		return;
	}
	else {
		string ip;
		while (!file.eof()) {
			getline(file,ip);
			AddUnicastIP(ip);
			if (std::find(ipAddresses.begin(), ipAddresses.end(), ip) == ipAddresses.end()) {
				ipAddresses.push_back(ip);
			}
		}
	}
	file.close();
	cleanIPs();
}

void cleanIPs() {
	ipAddresses.erase(
		std::remove(
		ipAddresses.begin(),
		ipAddresses.end(),
		string("")
		),
		ipAddresses.end());
}

void cleanAndPrintIPs() {
	cleanIPs();
	cout << "Currently cached IP Addresses:" << endl;
	for (int i = 0; i < ipAddresses.size(); ++i) {
		cout << "IP " << i << ": " << ipAddresses.at(i) << endl;
	}
}

void removeFromIPs(string ip) {
	for (int i = 0; i < ipAddresses.size(); ++i) {
		if (ipAddresses.at(i) == ip) {
			ipAddresses.at(i) = "";
		}
	}
	cleanIPs();
}

void saveIPAddresses() {
	std::ofstream file("ips.txt");
	if (!file.good()) {
		return;
	}
	else {
		cleanIPs();
		for (int i = 0; i < ipAddresses.size(); ++i) {
			if (ipAddresses.at(i) != "") {
				file << ipAddresses.at(i);
			}
			if (i != ipAddresses.size() - 1) {
				file << endl;
			}
		}
	}
	file.close();
}

bool WINAPI ConsoleHandler(DWORD CEvent) {
	switch (CEvent) {
		case CTRL_CLOSE_EVENT:
			saveIPAddresses();
			break;
	}
	return true;
}

void printHelp() {
	cout << "r: reset" << endl;
	cout << "q: quit" << endl;
	cout << "p: print server info" << endl;
	cout << "d: refresh data descriptions and check for wiimotes" << endl;
	cout << "u: add unicast IP address" << endl;
	cout << "U: remove unicast IP address" << endl;
	cout << "i: print list of unicast IP addresses" << endl;
}

void setupMotive() {
	int iResult;
	int iConnectionType = ConnectionType_Multicast;

	// Create NatNet Client
	iResult = MotiveClient::CreateClient(iConnectionType);
	if (iResult != ErrorCode_OK)
	{
		printf("Error initializing client.  See log for details.  Exiting");
		getchar();
		exit(1);
	}
	else
	{
		printf("Client initialized and ready.\n");
	}

	// Send/receive test request
	printf("[VRServer3] Sending Test Request\n");
	void* response;
	int nBytes;
	iResult = MotiveClient::theClient->SendMessageAndWait("TestRequest", &response, &nBytes);
	if (iResult == ErrorCode_OK)
	{
		printf("[VRServer3] Received: %s", (char*)response);
	}
	MotiveClient::GetDataDescriptions();
	delete response;
	printf("\nServer is connected to Motive and listening for data...\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("== Holojam server =======---\n");

	BindIP::setIPAddressBinds();
	binder = new BindIP();
	forwardBinder = new BindIP();
	
	// Detects and connects to Wiimotes
	MotiveClient::checkForWiimotes();

	ipAddresses = vector<string>();
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) ConsoleHandler, true);

	// Protobuf setup
	//GOOGLE_PROTOBUF_VERIFY_VERSION;

	setupMotive();

	int c;
	bool bExit = false;
	int clientsI = 0;
	std::string in_str;

	// Start the packet serving thread
	thread packet_serving_thread(PacketServingThread);
	//thread packet_receiving_thread(PacketReceivingThread);

	initializeIPAddresses();

	while (!bExit)
	{
		printf("(press the 'h' key for help)\n");
		c = _getch();
		string ip = "";

		switch (c)
		{
		case 'u':
			cout << "\nEnter IP for connection: ";
			getline(cin, ip);
			AddUnicastIP(ip);
			ipAddresses.push_back(ip);
			cout << endl;
			break;
		case 'h':
			printHelp();
			break;
		case 'q':
			bExit = true;
			break;
		case 'r':
			MotiveClient::resetClient();
			break;
		case 'p':
			sServerDescription ServerDescription;
			memset(&ServerDescription, 0, sizeof(ServerDescription));
			MotiveClient::theClient->GetServerDescription(&ServerDescription);
			if (!ServerDescription.HostPresent)
			{
				printf("Unable to connect to server. Host not present. Exiting.");
				return 1;
			}
			break;
		case 'd':
			MotiveClient::checkForWiimotes();
			MotiveClient::GetDataDescriptions();
			break;
		case 'i':
			cleanAndPrintIPs();
			break;
		case 'U':
			cout << "\nEnter IP to delete from cache: ";
			getline(cin, ip);
			removeFromIPs(ip);
			break;
		default:
			printf("unrecognized keycode: %c", c);
			break;
		}
	}

	saveIPAddresses();

	// Done - clean up.
	{
		std::unique_lock < std::mutex > lck(PacketGroup::packet_groups_lock);
		//packet_receiving_thread.detach();
		packet_serving_thread.detach();
	}
	delete binder;
	delete forwardBinder;
	MotiveClient::theClient->Uninitialize();
	return ErrorCode_OK;
}