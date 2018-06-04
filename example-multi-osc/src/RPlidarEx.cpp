//
//  RPlidarEx.cpp
//
//  Created by Iwatani Nariaki on 2017/12/05.
//
//

#include "RPlidarEx.h"
#include "Utilities.h"
#include "imgui.h"
#include "ofxOscSender.h"

RPlidarEx::RPlidarEx()
{
	valid_area_.close();
}

ofJson RPlidarEx::toJson() const
{
	return ofxJsonUtils::create(
								"osc enabled", is_osc_enabled_,
								"osc address", osc_address_,
								"angle", angle_,
								"area points", valid_area_.getVertices(),
								"near threshold", near_threshold_,
								"persistence", persistence_,
								"max_distance", max_distance_);
}
void RPlidarEx::loadJson(const ofJson &json)
{
	ofxJsonUtils::load(json,
					   "osc enabled", is_osc_enabled_,
					   "osc address", osc_address_,
					   "angle", angle_,
					   "area points", valid_area_.getVertices(),
					   "near threshold", near_threshold_,
					   "persistence", persistence_,
					   "max_distance", max_distance_);
}

void RPlidarEx::drawImGui()
{
	const int buf_len = 256;
	char buf[buf_len]={};
	if(ImGui::TreeNode("osc")) {
		ImGui::Checkbox("enable", &is_osc_enabled_);
		strcpy(buf, osc_address_.c_str());
		if(ImGui::InputText("osc address", buf, buf_len, ImGuiInputTextFlags_EnterReturnsTrue)) {
			osc_address_ = buf;
		}
		ImGui::TreePop();
	}

	if(ImGui::TreeNode("area")) {
		ImGui::DragFloat("angle", &angle_);
		auto &vertices = valid_area_.getVertices();
		for(int i = 0; i < vertices.size(); ++i) {
			ImGui::PushID(i);
			if(ImGui::Button("insert")) {
				vertices.insert(begin(vertices)+i, vertices.empty()?ofPoint():vertices[i]);
			}
			ImGui::SameLine();
			ImGui::DragFloat2("pos", &vertices[i].x);
			ImGui::SameLine();
			if(ImGui::Button("delete")) {
				vertices.erase(begin(vertices)+i);
			}
			ImGui::PopID();
		}
		if(ImGui::Button("add")) {
			vertices.push_back(vertices.empty()?ofPoint():vertices.back());
		}
		ImGui::TreePop();
	}
	if(ImGui::TreeNode("tracker")) {
		ImGui::DragFloat("near threshold", &near_threshold_);
		if(ImGui::DragInt("persistence", &persistence_)) {
			tracker_.setPersistence(persistence_);
		}
		if(ImGui::DragFloat("max distance", &max_distance_)) {
			tracker_.setMaximumDistance(max_distance_);
		}
		ImGui::TreePop();
	}
	if(ImGui::TreeNode("tracked objects(read only)")) {
		for(auto &r : result_) {
			ImGui::Text("%4d : (%10.2f, %10.2f)", r.label, r.pos.x, r.pos.y);
		}
		ImGui::TreePop();
	}
}

string RPlidarEx::getSerialNumber() const
{
	return handle_->getSerialNumber();
}

void RPlidarEx::draw(const ofVec2f &center, float scale)
{
	ofPushMatrix();
	ofTranslate(center);
	ofScale(scale, scale);
	ofPushStyle();
	ofSetColor(ofColor::white);
	for(auto &r : result_) {
		ofDrawCircle(r.pos, 10);
		stringstream ss;
		ss << r.label << ":" << "(" << r.pos << ")";
		ofDrawBitmapStringHighlight(ss.str(), r.pos);
	}
	ofPopStyle();
	ofPopMatrix();
}
void RPlidarEx::debugDraw(const ofVec2f &center, float scale)
{
	ofPushMatrix();
	ofTranslate(center);
	ofPushStyle();
	ofSetColor(ofColor::white);
	ofDrawLine(-100,0,100,0);
	ofDrawLine(0,-100,0,100);
	ofScale(scale, scale);
	ofSetColor(ofColor::white);
	ofSetLineWidth(2);
	valid_area_.draw();
	ofPopStyle();
	for(auto &r : scanned_data_) {
		if(r.quality > 0) {
			ofVec2f pos = ofVec2f(r.distance, 0).getRotated(r.angle+angle_);
			ofPushStyle();
			ofFill();
			ofSetColor(ofColor::black);
			ofDrawCircle(pos, 10);
			ofNoFill();
			ofSetColor(ofColor::yellow);
			ofDrawCircle(pos, 10);
			ofPopStyle();
		}
	}
	ofPopMatrix();
}

vector<RPlidarEx::Result> RPlidarEx::update()
{
	if(!handle_->isConnected()) {
		scanned_data_.clear();
		result_.clear();
		return result_;
	}
	handle_->update();
	scanned_data_ = handle_->getResult();
	vector<cv::Point2f> points;
	points.reserve(scanned_data_.size());
	struct {
		vector<ofVec2f> points;
		ofVec2f getMiddle() {
			ofVec2f ret;
			for(auto &p:points){ret+=p;}
			return points.empty()?ofVec2f():ret/(float)points.size();
		}
	} sum;
	for(auto &r : scanned_data_) {
		if(r.quality > 0) {
			ofVec2f pos = ofVec2f(r.distance, 0).getRotated(r.angle+angle_);
			if(valid_area_.size() < 3 || valid_area_.inside(pos)) {
				if(!sum.points.empty() && sum.points.back().distance(pos) > near_threshold_) {
					points.push_back(ofxCv::toCv(sum.getMiddle()));
					sum.points.clear();
				}
				else {
					sum.points.push_back(pos);
				}
			}
		}
	}
	if(!sum.points.empty()) {
		points.push_back(ofxCv::toCv(sum.getMiddle()));
		sum.points.clear();
	}
	if(points.size() > 1) {
		auto a = points.front();
		auto b = points.back();
		auto sub = a-b;
		if(sub.x*sub.x+sub.y*sub.y < near_threshold_*near_threshold_) {
			points.pop_back();
			points[0] = cv::Point2f((a.x+b.x)/2.f, (a.y+b.y)/2.f);
		}
	}
	auto labels = tracker_.track(points);
	result_.resize(labels.size());
	for(int i = 0, num = labels.size(); i < num; ++i) {
		result_[i].label = labels[i];
		result_[i].pos = ofxCv::toOf(tracker_.getCurrent(labels[i]));
	}
	return result_;
}

void RPlidarEx::sendOsc(ofxOscSender &sender)
{
	for(auto &r : result_) {
		ofxOscMessage m;
		m.setAddress(osc_address_);
		m.addInt32Arg(r.label);
		m.addFloatArg(r.pos.x);
		m.addFloatArg(r.pos.y);
		sender.sendMessage(m);
	}
}

