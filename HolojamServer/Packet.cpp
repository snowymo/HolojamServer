// author: Zhenyi

#include "Packet.h"
#include <iostream>
#include <iomanip>

Packet::Packet()
{
	_totalSize = 0;
}

Packet::~Packet()
{

}

void Packet::set_mod_version(int mod_version)
{
	_mod_version = mod_version;
}

void Packet::set_time(int time)
{
	_timestamp = time;
}

void Packet::set_label(std::string label)
{
	_label = label;
}

int Packet::ByteSize()
{
	// member + liveObj count * liveobj size

	return _totalSize;
}

void Packet::write2stream()
{
	_stream.clear();
	_stream << _label;	// motive
	//_stream << _mod_version;
	//_stream << _timestamp;
	_stream >> _stringbuffer;
	_totalSize += _stringbuffer.size();
}

void Packet::addLiveObj(LiveObject* obj)
{
	obj->write2stream();
	_liveObjs.push_back(obj);
	_totalSize += obj->ByteSize();
}

const char * Packet::getStreamBuffer()
{
	//char * buffer = new char[ByteSize()];
	//_stream.write(buffer, ByteSize());
	std::string st = _stringbuffer;
	//_stream >> st;
	//std::string st(buffer);
	for (int i = 0; i < _liveObjs.size(); i++){
		st += _liveObjs[i]->_stringbuffer;;
	}
	return st.c_str();
}

std::string Packet::getStreamString()
{
	//char * buffer = new char[ByteSize()];
	//_stream.write(buffer, ByteSize());
	std::string st = _stringbuffer;
	//_stream >> st;
	//std::string st(buffer);
	for (int i = 0; i < _liveObjs.size(); i++){
		st += _liveObjs[i]->_stringbuffer;;
	}
	return st;
}

LiveObject::LiveObject()
{

}

LiveObject::LiveObject(std::string label, bool tracking_valid, float x, float y, float z, float qx, float qy, float qz, float qw, int button_bits, std::string extra_data)
:_x(x), _y(y), _z(z), _qx(qx), _qy(qy), _qz(qz), _qw(qw), _tracking_valid(tracking_valid), _label(label)
{
	_totalSize = 0;
}

LiveObject::~LiveObject()
{

}

int LiveObject::ByteSize()
{
	// member 
	
	return _totalSize;
}

void LiveObject::write2stream()
{
	float f = 0.5f;
	std::string sf;
	char *p = reinterpret_cast< char *>(&f);
	int l = sizeof(float);
	for (std::size_t i = 0; i != sizeof(float); i++){
		std::printf("0x%02X\n", p[i]);
	}
// 	_x = _y = _z = 999.9999f;
// 	_qx = _qy = _qz = _qw = 999.9999f;
	_stream.clear();
	// size of label + label + xyz qxqyqzqw with precision(4) + bool + int + string
	_stream << _label.size() << _label;
	//_stream << std::fixed << std::setprecision(4) << _x << _y << _z;
// 	_stream << "0x" << std::setfill('0') << std::setw(2) << _x;// << _y << _z;
// 	_stream << "0x" << std::setfill('0') << std::setw(4) << _x;// << _y << _z;
	_stream << _x << "f" << _y << "f" << _z << "f";
	_stream << _qx << "f" << _qy << "f" << _qz << "f" << _qw << "f";
	_stream << _tracking_valid;
	_stream << _button_bits;
	_stream << _extra_data;
	_stream << "="; // as an end
	//_totalSize = _stream.str().size();
	_stream >> _stringbuffer;
	_totalSize += _stringbuffer.size();
}

const char * LiveObject::getStreamBuffer()
{
	std::string st;
	_stream >> st;
	char * buffer = new char[ByteSize()];
	_stream.write(buffer, ByteSize());
	return st.c_str();
}
