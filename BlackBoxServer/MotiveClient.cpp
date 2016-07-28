#include "MotiveClient.h"
#include "PacketGroup.h"
#include <iostream>

using std::cout;
using std::endl;

/* MOTIVE */
NatNetClient* MotiveClient::theClient;
sDataDescriptions* MotiveClient::pDataDefs = NULL;
// Rigid body labels, etc.
map<int, string> MotiveClient::idToLabel;
map<std::string, wiimote*> MotiveClient::motes;
unsigned MotiveClient::detected = 0;
const unsigned int MAX_WIIMOTES = 7;

void MotiveClient::GetDataDescriptions() {
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
// Establish a NatNet Client connection
int MotiveClient::CreateClient(int iConnectionType)
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

// A NatNet packet has been received
void MotiveClient::HandleNatNetPacket(sFrameOfMocapData *data, void *pUserData)
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

/* Motive error handling */
void __cdecl MotiveClient::DataHandler(sFrameOfMocapData* data, void* pUserData)
{
	NatNetClient* pClient = (NatNetClient*)pUserData;
	HandleNatNetPacket(data, pUserData);
}
void __cdecl MotiveClient::MessageHandler(int msgType, char* msg)
{
	printf("\n%s\n", msg);
}
void MotiveClient::resetClient()
{
	int iSuccess;
	printf("\n\nre-setting Client\n\n.");
	iSuccess = theClient->Uninitialize();
	if (iSuccess != 0) {
		printf("error un-initting Client\n");
	}

	// Need some workaround to make this constant
	iSuccess = theClient->Initialize("127.0.0.1", "127.0.0.1");
	if (iSuccess != 0) {
		printf("error re-initting Client\n");
	}
}

// ADD YOUR WIIMOTE HARDWARE ADDRESSES HERE
char* MotiveClient::mote_id_to_label(QWORD id) {
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

void MotiveClient::checkForWiimotes() {

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
	while (detected < MAX_WIIMOTES)
	{
		wiimote *next = new wiimote;
		if (!next->Connect(wiimote::FIRST_AVAILABLE)) {
			break;
		}
		detected += 1;
		string label = mote_id_to_label(next->UniqueID);
		motes[label] = next;
		next->SetLEDs(0x0f);
		cout << "Connected to wiimote " << detected - 1 << next->UniqueID << endl;
		printf("\nname: %s", mote_id_to_label(next->UniqueID));
	}
	printf("\nNo more remotes found\n");
}