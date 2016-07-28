/*
BlackBoxServer 4.0
Includes some code from OptiTrack.
*/

#include "stdafx.h"
#include "PacketGroup.h"

using std::map;
using std::thread;

// ADD YOUR WIIMOTE HARDWARE ADDRESSES HERE
char *mote_id_to_label(QWORD id) {
	switch (id) {
	//case 0x9da2a2838788:
	//	return "VR5_wand";
	case 0x9898977f7f7d://0x9da09e838483:
		return "VR4_wand";
	case 0x9898977d7f7f:
		return "VR2_wand";
	case 0x96979d7c7d83:
		return "VR3_wand";
	case 0x9da2a2838788://0x9b9d9a828280:
		return "VR1_wand";
	default:
		return "???";
	}
}

map<std::string, wiimote*> motes;
unsigned detected = 0;

#pragma warning( disable : 4996 )

void __cdecl DataHandler(sFrameOfMocapData* data, void* pUserData);
// For NatNet error mesages
void __cdecl MessageHandler(int msgType, char* msg);
void resetClient();
int CreateClient(int iConnectionType);
int PacketServingThread();
int PacketReceivingThread();
void checkForWiimotes();

unsigned int MyServersDataPort = 1511;
unsigned int MyServersCommandPort = 1510;

NatNetClient* theClient;
FILE* fp;

sDataDescriptions* pDataDefs = NULL;
// Rigid body labels, etc.
map<int, string> idToLabel;
void GetDataDescriptions() {
	printf("\n\n[VRServer3] Requesting Data Descriptions...\n");
	int nBodies = theClient->GetDataDescriptions(&pDataDefs);
	if (!pDataDefs)
	{
		printf("[VRServer3] Unable to retrieve Data Descriptions.");
		return;
	}
	printf("[VRServer3] Received %d Data Descriptions.\n", pDataDefs->nDataDescriptions);
	for (int i = 0; i < pDataDefs->nDataDescriptions; i++)
	{
		if (pDataDefs->arrDataDescriptions[i].type == Descriptor_MarkerSet) {
			// TODO: Process descriptions for MarkerSets
		}
		else if (pDataDefs->arrDataDescriptions[i].type == Descriptor_RigidBody) {
			sRigidBodyDescription* pRB = pDataDefs->arrDataDescriptions[i].Data.RigidBodyDescription;
			idToLabel[pRB->ID] = pRB->szName;
		}
		else if (pDataDefs->arrDataDescriptions[i].type == Descriptor_Skeleton) {
			for (sRigidBodyDescription pRB : pDataDefs->arrDataDescriptions[i].Data.SkeletonDescription->RigidBodies) {
				idToLabel[pRB.ID] = pRB.szName;
			}
		}
		else {
			printf("Unknown data type.");
			continue;
		}
	}
}

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
		std::cout << "This version is not supported! - \n" << WSAGetLastError() << std::endl;
	}

	SOCKET soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	BOOL sockoptval = TRUE;
	int sockoptres = setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, (char*)&sockoptval, sizeof(BOOL));
	if (sockoptres != 0) {
		std::cout << "Error in Setting Socket Option: " << WSAGetLastError() << std::endl;
	}
	if (soc == INVALID_SOCKET)
		std::cout << "Creating socket fail\n";
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1615);
	addr.sin_addr.s_addr = INADDR_ANY;

	int conn = ::bind(soc, (sockaddr*)&addr, sizeof(addr));
	if (conn == SOCKET_ERROR){
		std::cout << "Error - when connecting " << WSAGetLastError() << std::endl;
		closesocket(soc);
		WSACleanup();
	}
	
	int len = 65507;
	char *buf = (char*)malloc(sizeof(char) * len);
	int flags = 0;

	Stream stream = Stream("224.1.1.1", 1612, true);
	
	while (true) {
		int addr_len = sizeof(addr);
		int recv_status = recvfrom(soc, buf, len, flags, (sockaddr*)&addr, &addr_len);
		if (recv_status == SOCKET_ERROR) {
			std::cout << "Error in Receiving: " << WSAGetLastError() << std::endl;
		}
		
		update_protocol_v3::Update *update = new update_protocol_v3::Update();
		update->ParseFromArray(buf, recv_status);

		stream.send(buf, update->ByteSize());
		Sleep(1);
	}
	
}
// A NatNet packet has been received
void HandleNatNetPacket(sFrameOfMocapData *data, void *pUserData)
{
	if (idToLabel.size() == 0) {
		return;
	}
	NatNetClient* pClient = (NatNetClient*)pUserData;
	bool bIsRecording = data->params & 0x01;
	bool bTrackedModelsChanged = data->params & 0x02;
	PacketGroup *pg = new PacketGroup((int)(data->fTimestamp * 1000), false, true, "motive");

	// Rigid Bodies
	for (int i = 0; i < data->nRigidBodies; i++)
	{
		sRigidBodyData rb = data->RigidBodies[i];

		int button_bits = 0;
		string label = idToLabel[rb.ID];
		if (motes.count(label) > 0) {
			motes[label]->RefreshState();
			button_bits = motes[label]->Button.Bits;
		}

		pg->addLiveObject(idToLabel[rb.ID],
			rb.params & 0x01, // tracking valid param
			rb.x, rb.y, rb.z,
			rb.qx, rb.qy, rb.qz, rb.qw,
			true,
			button_bits,
			"");
	}
	/* TODO: Implement Skeleton and Stray Markers later */
	/*
	for (int i = 0; i < data->nSkeletons; i++) {
		for (int j = 0; j < data->Skeletons[i].nRigidBodies; j++) {
			sRigidBodyData rb = data->Skeletons[i].RigidBodyData[j];
			pg->addLiveObject(idToLabel[rb.ID],
				rb.params & 0x01, // tracking valid param
				rb.x, rb.y, rb.z,
				rb.qx, rb.qy, rb.qz, rb.qw,
				true,
				0,
				"");
		}
	}

	for (int i = 0; i < data->nOtherMarkers; i++) {
		float* m = data->OtherMarkers[i];
		pg->addLiveObject("marker",
			1, // tracking valid param
			m[0], m[1], m[2],
			0, 0, 0, 1,
			true,
			0);
		
	}
	//*/
	PacketGroup::queueHead(pg);
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("== Holojam server =======---\n");

	// Detects and connects to Wiimotes
	checkForWiimotes();

	// Protobuf setup
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	int iResult;
	int iConnectionType = ConnectionType_Multicast;

	// Create NatNet Client
	iResult = CreateClient(iConnectionType);
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
	iResult = theClient->SendMessageAndWait("TestRequest", &response, &nBytes);
	if (iResult == ErrorCode_OK)
	{
		printf("[VRServer3] Received: %s", (char*)response);
	}
	GetDataDescriptions();

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
			resetClient();
			break;
		case 'p':
			sServerDescription ServerDescription;
			memset(&ServerDescription, 0, sizeof(ServerDescription));
			theClient->GetServerDescription(&ServerDescription);
			if (!ServerDescription.HostPresent)
			{
				printf("Unable to connect to server. Host not present. Exiting.");
				return 1;
			}
			break;
		case 'd':
			checkForWiimotes();
			GetDataDescriptions();
			break;
		default:
			printf("unrecognized keycode: %c", c);
			break;
		}
	}

	// Done - clean up.
	packet_receiving_thread.detach();
	packet_serving_thread.detach();
	theClient->Uninitialize();
	return ErrorCode_OK;
}

void checkForWiimotes() {

	if (!motes.empty()) {

		auto itr = motes.begin();
		while (itr != motes.end()) {
			auto toErase = itr;
			++itr;
			delete(toErase->second);
			motes.erase(toErase);
		}
	}

	/* WiiMotes */
	printf("\nLooking for wiimotes...");
	detected = 0;
	while (detected < 7)
	{
		wiimote *next = new wiimote;
		if (!next->Connect(wiimote::FIRST_AVAILABLE)) {
			break;
		}
		detected += 1;
		string label = mote_id_to_label(next->UniqueID);
		motes[label] = next;
		next->SetLEDs(0x0f);
		printf("\nConnected to wiimote #%u: %" PRIx64, detected - 1, next->UniqueID);
		printf("\nname: %s", mote_id_to_label(next->UniqueID));
	}
	printf("\nNo more remotes found\n");
}



/* MOTIVE */
// Establish a NatNet Client connection
int CreateClient(int iConnectionType)
{
	// release previous server
	if (theClient)
	{
		theClient->Uninitialize();
		delete theClient;
	}

	// create NatNet client
	theClient = new NatNetClient(iConnectionType);
	unsigned char ver[4];
	theClient->NatNetVersion(ver);
	printf("(NatNet ver. %d.%d.%d.%d)\n", ver[0], ver[1], ver[2], ver[3]);

	theClient->SetMessageCallback(MessageHandler);
	theClient->SetVerbosityLevel(Verbosity_Error);
	theClient->SetDataCallback(DataHandler, theClient);

	// Init Client and connect to NatNet server
	// to use NatNet default port assigments
	int retCode = theClient->Initialize("127.0.0.1", "127.0.0.1");

	if (retCode != ErrorCode_OK)
	{
		printf("Unable to connect to server.  Error code: %d. Exiting", retCode);
		return ErrorCode_Internal;
	}
	else
	{
		// Print server info
		sServerDescription ServerDescription;
		memset(&ServerDescription, 0, sizeof(ServerDescription));
		theClient->GetServerDescription(&ServerDescription);

		printf("Motive Server application info:\n");
		printf("Application: %s (ver. %d.%d.%d.%d)\n", ServerDescription.szHostApp, ServerDescription.HostAppVersion[0],
			ServerDescription.HostAppVersion[1], ServerDescription.HostAppVersion[2], ServerDescription.HostAppVersion[3]);
		printf("NatNet Version: %d.%d.%d.%d\n", ServerDescription.NatNetVersion[0], ServerDescription.NatNetVersion[1],
			ServerDescription.NatNetVersion[2], ServerDescription.NatNetVersion[3]);
		printf("Client IP:%s\n", "192.168.0.1");
		printf("Server IP:%s\n", "192.168.0.1");
		printf("Server Name:%s\n\n", ServerDescription.szHostComputerName);
	}

	return ErrorCode_OK;

}

/* Motive error handling */
void __cdecl DataHandler(sFrameOfMocapData* data, void* pUserData)
{
	NatNetClient* pClient = (NatNetClient*)pUserData;
	HandleNatNetPacket(data, pUserData);
}
void __cdecl MessageHandler(int msgType, char* msg)
{
	printf("\n%s\n", msg);
}
void resetClient()
{
	int iSuccess;
	printf("\n\nre-setting Client\n\n.");
	iSuccess = theClient->Uninitialize();
	if (iSuccess != 0) {
		printf("error un-initting Client\n");
	}

	iSuccess = theClient->Initialize("127.0.0.1", "127.0.0.1");
	if (iSuccess != 0) {
		printf("error re-initting Client\n");
	}
}