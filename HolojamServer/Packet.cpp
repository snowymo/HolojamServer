// author: Zhenyi

#include "Packet.h"
#include <iostream>

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

// 	for (int i = 0; i < _liveObjs.size(); i++){
// 		_totalSize += _liveObjs[i]->ByteSize();
// 	}

	return _totalSize;
}

void Packet::write2stream()
{
	_stream.clear();
	_stream << _label;
	_stream << _mod_version;
	_stream << _timestamp;
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
		//const char * b = 
		//std::string stemp(b);
		st += _liveObjs[i]->_stringbuffer;;
	}
	std::cout << "check st:" << st << "\n";
	std::cout << "check buf:" << st.c_str() << "\n";
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
		//const char * b = 
		//std::string stemp(b);
		st += _liveObjs[i]->_stringbuffer;;
	}
	std::cout << "check st:" << st << "\n";
	//std::cout << "check buf:" << st.c_str() << "\n";
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
	//_stream.write(reinterpret_cast<const char*>(_x), 4);
	_stream.clear();
	_stream << "label:" << _label;
	_stream << "x:" << _x << ",y:"<< _y << ",z:" << _z;
	_stream << _qx << _qy << _qz << _qw;
	_stream << _tracking_valid;
	_stream << _button_bits;
	_stream << _extra_data;
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
