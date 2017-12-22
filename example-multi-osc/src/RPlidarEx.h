//
//  RPlidarEx.h
//
//  Created by Iwatani Nariaki on 2017/12/05.
//
//

#pragma once

#include "ofxRPlidar.h"
#include "ofRectangle.h"
#include "ofGraphics.h"
#include "ofVec2f.h"
#include "ofPolyline.h"
#include <vector>
#include "Tracker.h"
#include "ofxJsonUtils.h"

class ofxOscSender;

class RPlidarEx
{
public:
	RPlidarEx();
	struct Result {
		int label;
		ofVec2f pos;
	};
	void debugDraw(const ofVec2f &center, float scale=1);
	void draw(const ofVec2f &center, float scale=1);
	void drawImGui();
	bool isOscEnabled() const { return is_osc_enabled_; }
	void sendOsc(ofxOscSender &sender);
	ofJson toJson() const;
	void loadJson(const ofJson &json);
	std::vector<Result> update();
	std::string getSerialPath() const { return serial_path_; }
	std::string getSerialNumber() const;
	void setHandle(std::shared_ptr<ofxRPlidar> handle) { handle_ = handle; }
private:
	std::shared_ptr<ofxRPlidar> handle_;
	std::string serial_path_;
	bool is_osc_enabled_;
	std::string osc_address_;
	float angle_=0;
	ofPolyline valid_area_;
	float near_threshold_=0;
	int persistence_=0;
	float max_distance_=0;
	ofxCv::PointTracker tracker_;
	
	vector<ofx::rplidar::device::A2::ScannedData> scanned_data_;
	std::vector<Result> result_;
};
