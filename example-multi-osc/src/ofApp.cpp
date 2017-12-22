#include "ofApp.h"
#include "imgui.h"
#include "ofxJsonUtilsUtilsEx_ImGui.h"

template<> template<>
bool ofxJsonContainer<ofApp::OscSettings>::userFunc()
{
	bool ret = false;
	const int buf_len = 256;
	char buf[buf_len]={};
	strcpy(buf, host.c_str());
	if(ImGui::InputText("host", buf, buf_len, ImGuiInputTextFlags_EnterReturnsTrue)) {
		host = buf;
		ret = true;
	}
	if(ImGui::InputInt("port", &port)) {
		ret = true;
	}
	return ret;
}

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(0);
	ofSetFrameRate(60);
	sensors_.setDidLoadListener([this](std::map<std::string, std::shared_ptr<RPlidarEx>> &sensors, const ofJson &json) {
		for(auto it = begin(sensors); it != end(sensors);) {
			auto found = sensor_instances_.find(it->first);
			if(found != end(sensor_instances_)) {
				it->second->setHandle(found->second);
				++it;
			}
			else {
				it = sensors.erase(it);
			}
		}
	});
	sensors_.load("sensors.json");
	rescan();
	
	osc_.setDidLoadListener([this](OscSettings &s, const ofJson &json) {
		osc_sender_.setup(s.host, s.port);
	});
	osc_.load("osc.json");

	gui_.setup();
}

void ofApp::rescan()
{
	sensors_.clear();
	sensor_instances_.clear();
	auto sensor_list = ofxRPlidar::getDeviceList();
	for(auto &sensor_info : sensor_list) {
		string path = sensor_info.getDevicePath();
		auto sensor = make_shared<ofxRPlidar>();
		if(sensor->connect(path) && sensor->start()) {
			string serial_number = sensor->getSerialNumber();
			sensor_instances_.insert(make_pair(serial_number, sensor));
			sensors_.insert(make_pair(serial_number, make_shared<RPlidarEx>()));
		}
	}
	sensors_.reload();
}

//--------------------------------------------------------------
void ofApp::update(){
	for(auto &s : sensors_) {
		s.second->update();
		if(s.second->isOscEnabled()) {
			s.second->sendOsc(osc_sender_);
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofVec2f center = ofVec2f(ofGetWidth(), ofGetHeight())/2.f;
	for(auto &s : sensors_) {
		if(is_preview_[s.first]) {
			s.second->debugDraw(center, preview_scale_);
			s.second->draw(center, preview_scale_);
		}
	}
	gui_.begin();
	if(ImGui::Begin("sensors")) {
		ImGui::SliderFloat("preview scale", &preview_scale_, 0, 1);
		if(ImGui::Button("rescan")) {
			rescan();
		}
		ofxJsonImGui::SaveLoad(sensors_, "sensors.json", 4);
		for(auto &s : sensors_) {
			auto device = [this](const string &serial_number) -> shared_ptr<ofxRPlidar> {
				shared_ptr<ofxRPlidar> ret;
				auto found = sensor_instances_.find(serial_number);
				if(found != end(sensor_instances_)) {
					return found->second;
				}
				return nullptr;
			}(s.first);
			if(device) {
				if(ImGui::TreeNode(s.first.c_str())) {
					if(ImGui::Button("reconnect")) {
						if(device->reconnect()) {
							device->start();
						}
					}
					ImGui::SameLine();
					if(ImGui::Button("stop")) {
						device->stop();
					}
					ImGui::Checkbox("preview", &is_preview_[s.first]);
					s.second->drawImGui();
					ImGui::TreePop();
				}
			}
		}
	}
	ImGui::End();
	if(ImGui::Begin("osc")) {
		ofxJsonImGui::SaveLoad(osc_, "osc.json", 4);
		if(osc_.userFunc<bool>()) {
			osc_sender_.setup(osc_.host, osc_.port);
		}
	}
	ImGui::End();
	gui_.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
