#pragma once
#include "../Include/NatNetTypes.h"
#include "../Include/NatNetClient.h"
#include "PacketGroup.h"
#include <map>
#include <string>
#include "wiimote.h"

using std::map;
using std::string;

/* MOTIVE */

class MotiveClient {
	// Establish a NatNet Client connection
	public:
		static NatNetClient* theClient;
		static map<string, wiimote*> motes;
		static unsigned detected;

		static sDataDescriptions* pDataDefs;
		// Rigid body labels, etc.
		static map<int, string> idToLabel;
		static void GetDataDescriptions();
		static int CreateClient(int iConnectionType);

		/* Motive error handling */
		static void __cdecl DataHandler(sFrameOfMocapData* data, void* pUserData);
		static void __cdecl MessageHandler(int msgType, char* msg);

		static void resetClient();
		static void HandleNatNetPacket(sFrameOfMocapData *data, void *pUserData);
		// ADD YOUR WIIMOTE HARDWARE ADDRESSES HERE
		static char *mote_id_to_label(QWORD id);
		static void checkForWiimotes();
};