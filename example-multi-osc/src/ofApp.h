#pragma once

#include "ofMain.h"
#include "ofxRPlidar.h"
#include "RPlidarEx.h"
#include "ofxJsonUtilsUtils.h"
#include "ofxJsonUtilsUtilsContainer.h"
#include "ofxImGui.h"
#include "ofxOscSender.h"

class ofApp : public ofBaseApp{
	
public:
	void setup();
	void rescan();
	void update();
	void draw();
	
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	struct OscSettings {
		std::string host;
		int port;
		JSON_FUNCS(host,port);
	};
private:
	ofxJsonContainer<std::map<std::string, std::shared_ptr<RPlidarEx>>> sensors_;
	std::map<std::string, std::shared_ptr<ofxRPlidar>> sensor_instances_;
	ofxImGui::Gui gui_;
	float preview_scale_=1;
	std::map<std::string, bool> is_preview_;
	
	ofxOscSender osc_sender_;	
	ofxJsonContainer<OscSettings> osc_;
};
