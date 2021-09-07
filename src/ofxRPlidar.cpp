/*
MIT LICENSE

Copyright 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ofxRPlidar.h"
#include "ofLog.h"
#include "ofUtils.h"
#include "ofSerial.h"


using namespace rp::standalone::rplidar;
using namespace ofx::rplidar;
using namespace std;

namespace {
	bool isDeviceRplidar(ofSerialDeviceInfo &device) {
#if defined(TARGET_WIN32)
		return ofIsStringInString(device.getDeviceName(), "Silicon Labs CP210x USB to UART Bridge");
#elif defined(TARGET_LINUX)
		return ofIsStringInString(device.getDeviceName(), "ttyUSB0") || ofIsStringInString(device.getDeviceName(), "ttyUSB1");
#else
		return ofIsStringInString(device.getDeviceName(), "tty.SLAB_USBtoUART");
	#endif
	}
}
vector<ofSerialDeviceInfo> device::A2::getDeviceList()
{
	ofSerial serial;
	auto ret = serial.getDeviceList();
	ret.erase(remove_if(begin(ret), end(ret), [](ofSerialDeviceInfo &info){
		return !isDeviceRplidar(info);
	}), end(ret));
	return ret;
}

device::A2::A2()
{
	// create the driver instance
	driver_ = RPlidarDriver::CreateDriver(DRIVER_TYPE_SERIALPORT);
	
	if (!driver_) {
		ofLogError("RPLIDAR", "insufficent memory, exit");
	}
}

device::A2::~A2()
{
	disconnect();
	if(driver_) {
		RPlidarDriver::DisposeDriver(driver_);
	}
}

bool device::A2::connect(const string &serial_path, int baud_rate)
{
	u_result op_result;
	
	serial_path_ = serial_path;
	// try to connect
	if (IS_FAIL(driver_->connect(serial_path.c_str(), baud_rate))) {
		ofLogError("RPLIDAR", "Error, cannot bind to the specified serial port %s.\n", serial_path.c_str());
		return false;
	}
	
	// retrieving the device info
	////////////////////////////////////////
	op_result = driver_->getDeviceInfo(device_info_);
	
	if (IS_FAIL(op_result)) {
		if (op_result == RESULT_OPERATION_TIMEOUT) {
			// you can check the detailed failure reason
			ofLogError("RPLIDAR", "Error, operation time out.");
		} else {
			ofLogError("RPLIDAR", "Error, unexpected error, code: %x", op_result);
			// other unexpected result
		}
		return false;
	}
	
	// print out the device serial number, firmware and hardware version number..
	string serial_number = getSerialNumber();
	ofLogVerbose("RPLIDAR", "Serial Number: %s", serial_number.c_str());
	
	ofLogVerbose("RPLIDAR", "Version: %s", RPLIDAR_SDK_VERSION);
	ofLogVerbose("RPLIDAR", "Firmware Ver: %d.%02d", device_info_.firmware_version>>8, device_info_.firmware_version & 0xFF);
	ofLogVerbose("RPLIDAR", "Hardware Rev: %d", (int)device_info_.hardware_version);
	
	
	// check the device health
	////////////////////////////////////////
	op_result = driver_->getHealth(health_info_);
	if (IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
		ofLogVerbose("RPLIDAR", "health status: %s", [](_u8 status) {
			switch (status) {
				case RPLIDAR_STATUS_OK:
					return "OK.";
				case RPLIDAR_STATUS_WARNING:
					return "Warning.";
				case RPLIDAR_STATUS_ERROR:
					return "Error.";
			}
		}(health_info_.status));
		ofLogVerbose("RPLIDAR", " (errorcode: %d)", health_info_.error_code);
	} else {
		ofLogError("RPLIDAR", "Error, cannot retrieve the lidar health code: %x", op_result);
		return false;
	}
	
	if (health_info_.status == RPLIDAR_STATUS_ERROR) {
		ofLogError("ROLIDAR", "Error, rplidar internal error detected. Please reboot the device to retry.");
		// enable the following code if you want rplidar to be reboot by software
		// drv->reset();
		return false;
	}

	// Get device scan modes
	driver_->getAllSupportedScanModes(scanModes);
	scanMode = 0;

	return true;
}

void device::A2::setScanMode(int scanMode_) {
	scanMode = scanMode_;
}

bool device::A2::reconnect(int baud_rate)
{
	return connect(serial_path_, baud_rate);
}

bool device::A2::disconnect()
{
	if(isConnected()) {
		stop();
		driver_->disconnect();
		return true;
	}
	return false;
}

bool device::A2::isConnected() const
{
	return driver_ && driver_->isConnected();
}

bool device::A2::start(bool threaded)
{
	if(isConnected()
	   && !IS_FAIL(driver_->startMotor())
     && !IS_FAIL(driver_->startScan(false, true, 0, &scanModes[scanMode]))) {
		if(threaded) {
			startThread();
		}
		return true;
	}
	return false;
}
bool device::A2::stop()
{
	if(isThreadRunning()) {
		stopThread();
		waitForThread();
	}
	if(isConnected()
	   && !IS_FAIL(driver_->stop())
	   && !IS_FAIL(driver_->stopMotor())) {
		return true;
	}
	return false;
}

void device::A2::threadedFunction()
{
	while(isThreadRunning()) {
		result_.back() = scan(true);
		lock();
		has_new_frame_ = true;
		result_.swap();
		unlock();
		ofSleepMillis(1);
		if(!isConnected()) {
			stopThread();
		}
	}
}

void device::A2::update()
{
	bool new_frame = false;
	if(isThreadRunning()) {
		lock();
		new_frame = has_new_frame_;
		has_new_frame_ = false;
		unlock();
	}
	else {
		result_.front() = scan(true);
		new_frame = true;
	}
	is_frame_new_ = new_frame;
}

vector<device::A2::ScannedData> device::A2::getResult()
{
	if(isThreadRunning()) {
		lock();
		vector<device::A2::ScannedData> ret = result_.front();
		unlock();
		return ret;
	}
	else {
		return result_.front();
	}
}

string device::A2::getSerialNumber() const
{
	string ret;
	for (int pos = 0; pos < 16 ;++pos) {
		ret += ofToHex(device_info_.serialnum[pos]);
	}
	return ret;
}

vector<device::A2::ScannedData> device::A2::scan(bool ascend)
{
	vector<ScannedData> ret;
	
	rplidar_response_measurement_node_t nodes[360*2];
	size_t count = sizeof(nodes)/sizeof(rplidar_response_measurement_node_t);
	
	u_result ans = driver_->grabScanData(nodes, count);
	if (IS_OK(ans) || ans == RESULT_OPERATION_TIMEOUT) {
		if(ascend) {
			driver_->ascendScanData(nodes, count);
		}
		ret.resize(count);
		for (int i = 0; i < count ; ++i) {
			ScannedData &data = ret[i];
			data.sync = (nodes[i].sync_quality & RPLIDAR_RESP_MEASUREMENT_SYNCBIT) != 0;
			data.angle = (nodes[i].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)/64.0f;
			data.distance = nodes[i].distance_q2/4.0f;
			data.quality = nodes[i].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
		}
	} else {
		ofLogError("RPLIDAR", "error code: %x", ans);
	}
	return ret;
}

