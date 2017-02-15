#ifndef _PACKET_H
#define _PACKET_H

#include <string>
#include <sstream>
#include <vector>

class LiveObject
{
public:

	LiveObject();
	LiveObject(std::string label, bool tracking_valid, float x, float y, float z, float qx, float qy, float qz, float qw, int button_bits, std::string extra_data);
	~LiveObject();

	std::string getLabel() const { return _label; }
	void setLabel(std::string val) { _label = val; }
	float getX() const { return _x; }
	void setX(float val) { _x = val; }
	float getY() const { return _y; }
	void setY(float val) { _y = val; }
	float getZ() const { return _z; }
	void setZ(float val) { _z = val; }
	float getQx() const { return _qx; }
	void setQx(float val) { _qx = val; }
	float getQy() const { return _qy; }
	void setQy(float val) { _qy = val; }
	float getQz() const { return _qz; }
	void setQz(float val) { _qz = val; }
	float getQw() const { return _qw; }
	void setQw(float val) { _qw = val; }
	bool getTracking_valid() const { return _tracking_valid; }
	void setTracking_valid(bool val) { _tracking_valid = val; }
	int getButton_bits() const { return _button_bits; }
	void setButton_bits(int val) { _button_bits = val; }
	std::string getExtra_data() const { return _extra_data; }
	void setExtra_data(std::string val) { _extra_data = val; }

	int ByteSize() ;

	void write2stream();
	const char *getStreamBuffer();

private:
	std::string _label;
	float _x;
	float _y;
	float _z;
	float _qx;
	float _qy;
	float _qz;
	float _qw;
	bool _tracking_valid;
	int _button_bits;
	std::string _extra_data;

	int _totalSize;

	std::stringstream _stream;
public:
	std::string _stringbuffer;
};

class Packet{
private:
	int _mod_version;
	int _timestamp;
	std::string _label;
	bool _lhs;

	std::stringstream _stream;
public:

	std::string _stringbuffer;
	int _totalSize;
	std::vector<LiveObject*> _liveObjs;

public:
	Packet();
	~Packet();

	void set_mod_version(int mod_version);
	void set_time(int time);
	void set_label(std::string label);
	bool getLhs() const { return _lhs; }
	void setLhs(bool val) { _lhs = val; }

	int ByteSize() ;
	void write2stream();
	void addLiveObj(LiveObject* obj);
	
	const char *getStreamBuffer();
	std::string getStreamString();
};

#endif
