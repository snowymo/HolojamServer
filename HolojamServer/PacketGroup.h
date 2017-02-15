#pragma once
#ifndef __PACKETGROUP__H
#define __PACKETGROUP__H

#include <condition_variable>
#include <vector>
#include <mutex>
#include <memory>

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "update_protocol_v3.pb.h"
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/arena.h>

#include "Packet.h"

using std::string;
using std::vector;
using std::mutex;
using std::unique_ptr;

class Stream;

class PacketGroup {

	private:
		/* static fields */
		const static int max_packet_bytes = 1300;
		static vector<PacketGroup*> packet_groups;

		// This static field should only be accessed by PacketGroup instances
		// It should not be accessed by the static class, multiple threads,
		// or more than one packet group... for now.
		static int mod_version;
		/* instance fields */
		//TODO
		//google::protobuf::Arena arena;
		//vector<update_protocol_v3::Update * > packets;
		vector<Packet*> purePackets;
		//vector<update_protocol_v3::Update * > ::iterator next_packet;
		vector<Packet*>::iterator next_purePacket;
		int timestamp;
		bool recording;
		bool models_changed;
		string label;
		bool all_sent;
		/* private instance methods */
		update_protocol_v3::Update *newPacket();
		Packet* newPurePacket();

	public:
		/* PacketGroup instance methods allow a user to construct and fill a PacketGroup.
		* After a PacketGroup is constructed it is set as the latest packet group using setHead.
		* When a PacketGroup is set as the latest/head packet group it should not be modified and
		* its instance methods should no longer be invoked by an outside class.
		*/

		static std::mutex packet_groups_lock;
		static std::condition_variable cv;
		static char buffer[max_packet_bytes];
		PacketGroup(int _timestamp, bool _recording, bool _models_changed, string _label,bool v=false);
		void addPacket(update_protocol_v3::Update *packet);
		void addPacket(Packet* packet);
		//void addLiveObject(update_protocol_v3::LiveObject o, bool lhs);
		void addLiveObject(string label, bool tracking_valid, float x, float y, float z, float qx, float qy, float qz, float qw, bool lhs, int button_bits, string extra_data);
		void addLiveObject(LiveObject* obj, bool lhs);
		update_protocol_v3::Update *getNextPacketToSend();
		Packet* getNextPacket2Send();
		static void send(Stream* multicast_stream, vector<Stream*>* unicast_streams);
		static void queueHead(PacketGroup *newHead);
		static void AddUnicastIP(string ip, vector<Stream*>* unicast_streams);
		~PacketGroup();
};
#endif