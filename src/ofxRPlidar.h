/*
MIT LICENSE

Copyright 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <string>
#include "ofTypes.h"
#include "ofThread.h"
#include "rplidar.h" //RPLIDAR standard sdk, all-in-one header

namespace rp { namespace standalone { namespace rplidar {
class RPlidarDriver;
}}}
namespace ofx {
namespace rplidar {
namespace device {
class A2 : ofThread
{
public:
	struct ScannedData {
		float angle;
		float distance;
		unsigned char quality;
		bool sync;
	};
	A2();
	virtual ~A2();
	static std::vector<ofSerialDeviceInfo> getDeviceList();
	bool connect(const std::string &serial_path, int baud_rate=115200);
	bool reconnect(int baud_rate=115200);
	bool disconnect();
	bool isConnected() const;
	bool start(bool threaded=true);
	bool stop();
	std::vector<ScannedData> scan(bool ascend=true);
	std::vector<ScannedData> getResult();
	std::string getSerialPath() const { serial_path_; }
	std::string getSerialNumber() const;
protected:
	std::string serial_path_;
	void threadedFunction();
	std::vector<ScannedData> result_;
	rp::standalone::rplidar::RPlidarDriver *driver_;
	rplidar_response_device_info_t device_info_;
	rplidar_response_device_health_t health_info_;
};
}
}
}

using ofxRPlidar = ofx::rplidar::device::A2;
