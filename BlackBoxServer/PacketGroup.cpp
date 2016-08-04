#include "PacketGroup.h"
#include "BindIP.h"
#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::getline;
using std::cin;

/* Initialize PacketGroup static fields */
const int max_packet_bytes = 1300;
vector<PacketGroup*> PacketGroup::packet_groups = vector<PacketGroup*>();
int PacketGroup::mod_version = 0;
mutex PacketGroup::packet_groups_lock;
char PacketGroup::buffer[max_packet_bytes];

//Stream PacketGroup::multicast_stream = Stream(MULTICAST_IP.c_str(), PORT, true);
//vector<unique_ptr<Stream> > PacketGroup::unicast_streams = vector<unique_ptr<Stream> >();
void PacketGroup::AddUnicastIP(string ip, vector<Stream*>* unicast_streams) {
	for (int i = 0; i < unicast_streams->size(); ++i) {
		if (unicast_streams->at(i)->getIP() == ip) {
			return;
		}
	}
	Stream* stream = new Stream(ip.c_str(), PORT, false);
	unicast_streams->push_back(stream);
}

update_protocol_v3::Update* PacketGroup::newPacket() {
	update_protocol_v3::Update *packet = new update_protocol_v3::Update();
	addPacket(packet);
	return packet;
}

PacketGroup::PacketGroup(int _timestamp, bool _recording, bool _models_changed, string _label) {
	timestamp = _timestamp;
	recording = _recording;
	models_changed = _models_changed;
	label = _label;
	all_sent = false;
	packets = vector<update_protocol_v3::Update * >();
	// Add the first packet
	update_protocol_v3::Update *packet = newPacket();
	assert(packet->ByteSize() < max_packet_bytes);
	// Start the iterator
	next_packet = packets.begin();
}
void PacketGroup::addPacket(update_protocol_v3::Update *packet) {
	packet->set_mod_version(mod_version++);
	packet->set_time(timestamp);
	packet->set_label(label);
	packets.push_back(packet);
	next_packet = packets.begin();
}

void PacketGroup::addLiveObject(update_protocol_v3::LiveObject o, bool lhs) {
	addLiveObject(o.label(), true, o.x(), o.y(), o.z(), o.qx(), o.qy(), o.qz(), o.qw(), lhs, o.button_bits(), o.extra_data());
}

void PacketGroup::addLiveObject(string label, bool tracking_valid, float x, float y, float z, float qx, float qy, float qz, float qw, bool lhs, int button_bits, string extra_data) {
	update_protocol_v3::LiveObject *liveObj = new update_protocol_v3::LiveObject();

	liveObj->set_label(label);

	liveObj->set_x(x);
	liveObj->set_y(y);
	liveObj->set_z(z);

	liveObj->set_qx(qx);
	liveObj->set_qy(qy);
	liveObj->set_qz(qz);
	liveObj->set_qw(qw);
	liveObj->set_is_tracked(tracking_valid);

	liveObj->set_button_bits(button_bits);
	liveObj->set_extra_data(extra_data);

	update_protocol_v3::Update *current_packet = packets.back();
	if (!(current_packet->ByteSize() + liveObj->ByteSize() < max_packet_bytes)) {
		// Create a new packet to hold the rigid body
		current_packet = newPacket();
	}
	current_packet->set_lhs_frame(lhs);
	assert(current_packet->ByteSize() + liveObj->ByteSize() < max_packet_bytes + 100);
	current_packet->mutable_live_objects()->AddAllocated(liveObj);
	assert(current_packet->ByteSize() < max_packet_bytes + 100);
}

update_protocol_v3::Update* PacketGroup::getNextPacketToSend() {
	assert(packets.size() > 0);
	update_protocol_v3::Update *packet = *next_packet;
	// Step forward and reset to the beginning if we are at the end
	next_packet++;
	if (next_packet == packets.end()) {
		all_sent = true;
		next_packet = packets.begin();
	}
	return packet;
}

/* Packet group oriented methods */
/* These methods are thread safe and operator on packets that should not be modified */
void PacketGroup::send(Stream* multicast_stream, vector<Stream*>* unicast_streams) {
	// Ensure a packet group exists to send
	if (packet_groups.size() == 0) {
		return;
	}
	packet_groups_lock.lock();

	PacketGroup *head = packet_groups.at(0);

	// Get the current packet of the packet group
	if (head->packets.size() == 0) {
		return;
	}
	update_protocol_v3::Update *packet = head->getNextPacketToSend();
	assert(packet->ByteSize() < max_packet_bytes + 128);
	packet->SerializePartialToArray(buffer, max_packet_bytes);

	// Send the buffer
	multicast_stream->send(buffer, packet->ByteSize());
	for (int i = 0; i < unicast_streams->size(); i++) {
		(*unicast_streams)[i]->send(buffer, packet->ByteSize());
	}
	if (head->all_sent) {
		packet_groups.push_back(head);
		packet_groups.erase(packet_groups.begin());
	}

	packet_groups_lock.unlock();
}

// Important: A PacketGroup must not be modified after it is set as the head
void PacketGroup::queueHead(PacketGroup *newHead) {
	packet_groups_lock.lock();

	vector<int> indexes_to_delete = vector<int>();
	// Find packet groups to delete
	for (int i = 0; i < packet_groups.size(); i++) {
		if (packet_groups[i]->label == newHead->label) {
			indexes_to_delete.push_back(i);
		}
	}
	// Delete them in backwards order
	for (int i = indexes_to_delete.size() - 1; i >= 0; i--) {
		int group_index = indexes_to_delete[i];
		PacketGroup *pg = packet_groups[group_index];
		for (int pi = pg->packets.size() - 1; pi >= 0; pi--) {
			update_protocol_v3::Update *p = pg->packets[pi];
			delete(p);
			pg->packets.erase(pg->packets.begin() + pi);
		}
		delete(packet_groups[group_index]);
		packet_groups.erase(packet_groups.begin() + group_index);
	}
	packet_groups.push_back(newHead);
	packet_groups_lock.unlock();
}