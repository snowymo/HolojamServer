/*
BlackBoxServer 4.0
Includes some code from OptiTrack.
*/

#include <iostream>
#include <thread>
#include <conio.h>

#include "targetver.h"
#include "PacketGroup.h"
#include "MotiveClient.h"
#include "update_protocol_v3.pb.h"
#include "Constants.h"

using std::thread;
using std::cout;
using std::endl;

#pragma warning( disable : 4996 )

int PacketServingThread();
int PacketReceivingThread();

FILE* fp;

int PacketServingThread() {
	while (true) {
		PacketGroup::send();
		Sleep(1);
	}
	return 0;
}

int PacketReceivingThread() {
	WSAData version;        //We need to check the version.
	WORD mkword = MAKEWORD(2, 2);
	int what = WSAStartup(mkword, &version);
	if (what != 0){
		cout << "This version is not supported! - \n" << WSAGetLastError() << endl;
	}

	SOCKET soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	BOOL sockoptval = TRUE;
	int sockoptres = setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, (char*)&sockoptval, sizeof(BOOL));
	if (sockoptres != 0) {
		cout << "Error in Setting Socket Option: " << WSAGetLastError() << endl;
	}
	if (soc == INVALID_SOCKET)
		cout << "Creating socket fail\n";
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SENDING_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	int conn = ::bind(soc, (sockaddr*)&addr, sizeof(addr));
	if (conn == SOCKET_ERROR){
		cout << "Error - when connecting " << WSAGetLastError() << endl;
		closesocket(soc);
		WSACleanup();
	}
	
	int len = 65507;
	char *buf = (char*)malloc(sizeof(char) * len);
	int flags = 0;

	Stream stream = Stream(MULTICAST_IP.c_str(), MULTICAST_SEND_PORT, true);
	
	while (true) {
		int addr_len = sizeof(addr);
		int recv_status = recvfrom(soc, buf, len, flags, (sockaddr*)&addr, &addr_len);
		if (recv_status == SOCKET_ERROR) {
			cout << "Error in Receiving: " << WSAGetLastError() << endl;
		}
		
		update_protocol_v3::Update *update = new update_protocol_v3::Update();
		update->ParseFromArray(buf, recv_status);

		stream.send(buf, update->ByteSize());
		Sleep(1);
	}
	
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("== Holojam server =======---\n");

	// Detects and connects to Wiimotes
	MotiveClient::checkForWiimotes();

	// Protobuf setup
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	int iResult;
	int iConnectionType = ConnectionType_Multicast;

	// Create NatNet Client
	iResult = MotiveClient::CreateClient(iConnectionType);
	if (iResult != ErrorCode_OK)
	{
		printf("Error initializing client.  See log for details.  Exiting");
		return 1;
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

	printf("\nServer is connected to Motive and listening for data...\n");
	int c;
	bool bExit = false;
	int clientsI = 0;
	std::string in_str;
	
	// Start the packet serving thread
	thread packet_serving_thread(PacketServingThread);
	thread packet_receiving_thread(PacketReceivingThread);

	while (!bExit)
	{
		printf("(press the 'h' key for help)\n");
		c = _getch();
		switch (c)
		{
		case 'h':
			printf("r: reset\nq: quit\np: print server info\nd: refresh data descriptions\n");
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
		default:
			printf("unrecognized keycode: %c", c);
			break;
		}
	}

	// Done - clean up.
	packet_receiving_thread.detach();
	packet_serving_thread.detach();
	MotiveClient::theClient->Uninitialize();
	return ErrorCode_OK;
}