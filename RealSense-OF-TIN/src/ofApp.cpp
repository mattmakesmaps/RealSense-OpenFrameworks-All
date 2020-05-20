#include "ofApp.h"
#include <string>
#include <iostream>

const int ofApp::depthFrameWidth = 848;
const int ofApp::depthFrameHeight = 480;
const int ofApp::appWidth = depthFrameWidth * 2;
const int ofApp::appHeight = depthFrameHeight * 2;
const int buffer = 0; // lets clip the outer edges to reduce noise.

namespace {
	bool filterNoise = false;
	auto connectDistance = 50;
	auto minRawDepth = 0.1;
	auto maxRawDepth = 2.0;
	auto minMappedDepth = 1;
	auto maxMappedDepth = 1000;
	int stepSize = 10;

	typedef std::pair <std::string, ofPrimitiveMode> primativePair;
	vector<primativePair> primativeModes = {
		primativePair("OF_PRIMITIVE_POINTS",OF_PRIMITIVE_POINTS),
		primativePair("OF_PRIMITIVE_TRIANGLES",OF_PRIMITIVE_TRIANGLES),
		primativePair("OF_PRIMITIVE_TRIANGLE_STRIP",OF_PRIMITIVE_TRIANGLE_STRIP),
		primativePair("OF_PRIMITIVE_TRIANGLE_FAN",OF_PRIMITIVE_TRIANGLE_FAN),
		primativePair("OF_PRIMITIVE_LINES",OF_PRIMITIVE_LINES),
		primativePair("OF_PRIMITIVE_LINE_STRIP",OF_PRIMITIVE_LINE_STRIP),
		primativePair("OF_PRIMITIVE_LINE_LOOP",OF_PRIMITIVE_LINE_LOOP)
	};

	vector<primativePair>::iterator primativeModeIterator = primativeModes.begin();
}

//--------------------------------------------------------------
void ofApp::setup() {
	rs2_pipe.start();
	ofSetVerticalSync(true);

	ofEnableDepthTest();
	glEnable(GL_POINT_SMOOTH); // use circular points instead of square points
	glPointSize(3); // make the points bigger

}

//--------------------------------------------------------------
void ofApp::update() {

	meshes.clear();

	// Block program until frames arrive
	rs2::frameset frames = rs2_pipe.wait_for_frames();
	// Try to get a frame of a depth image
	rs2::depth_frame depth = frames.get_depth_frame();

	// loop through the image in the x and y axes
	for (int y = 0 + buffer; y < depthFrameHeight - buffer; y += stepSize) {

		int vertCounter = 0;
		auto scanLine = std::make_unique<ofMesh>();
		scanLine->setMode(primativeModeIterator->second);
		scanLine->enableIndices();

		for (int x = 0 + buffer; x < depthFrameWidth - buffer; x += stepSize) {

			const ofColor pointColor = ofColor::orange;
			auto depthValue = depth.get_distance(x, y);

			// map depthValue to extrude it a bit
			auto extrudedDepthValue = ofMap(depthValue, minRawDepth, maxRawDepth, minMappedDepth, maxMappedDepth, false);
			scanLine->addColor(pointColor);
			glm::vec3 pos(x, y, extrudedDepthValue);
			// ignore floor/ceiling points
			if (!filterNoise || 
				(filterNoise && (extrudedDepthValue > minMappedDepth && extrudedDepthValue < maxMappedDepth)))
			{
				scanLine->addVertex(pos);
				scanLine->addIndex(vertCounter);
				vertCounter++;
			}
		}

		meshes.push_back(std::move(scanLine));
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackgroundGradient(ofColor::black, ofColor::black, OF_GRADIENT_CIRCULAR);

	// even points can overlap with each other, let's avoid that
	cam.begin();
	//ofScale(2, -2, 2); // flip the y axis
	//ofRotateYDeg(90);
	ofRotateZDeg(180);
	ofRotateXDeg(270);
	ofTranslate(-appWidth / 4 , 0, -appHeight/4);
	for (auto& scanLine : meshes) {
		scanLine->draw();
	}
	cam.end();

	// Draw Text
	stringstream ss;
	ss << "Point Density (m, n): " << stepSize << std::endl;
	ss << "minRawDepth (p,o): " << minRawDepth << std::endl;
	ss << "maxnRawDepth (l,k): " << maxRawDepth << std::endl;
	ss << "filterNoise (f): " << (filterNoise ? "true" : "false") << std::endl;
	ss << "connectDistance (r,t): " << connectDistance << std::endl;
	ss << "primativeMode (x): " << primativeModeIterator->first << std::endl;
	ofDrawBitmapString(ss.str().c_str(), 20, 20);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	// Toggle Filtering
	if (key == 'f')
		filterNoise = !filterNoise;

	// Cycle Primative Mode 
	if (key == 'x') {
		// MK NOTE: end() actually returns an iterator referring to the "past-the-end" element.
		// So I'm incrementing the iterator first, then checking if it is equivalent.
		primativeModeIterator++;
		if (primativeModeIterator == primativeModes.end()) {
			primativeModeIterator = primativeModes.begin();
		}
	}

	// Increase Decrease minRawDepth
	if (key == 'p') {
		if (minRawDepth <= 1) {
			minRawDepth += 0.05;
		}
		else {
			minRawDepth += 0.25;
		}
	}

	if (key == 'o') {
		if (minRawDepth > 1) {
			minRawDepth -= 0.25;
		}
		else if (minRawDepth > 0.05) {
			minRawDepth -= 0.05;
		}
	};

	// Increase Decrease maxRawDepth 
	if (key == 'l') {
		if (maxRawDepth <= 1) {
			maxRawDepth += 0.05;
		}
		else {
			maxRawDepth += 0.25;
		}
	}

	if (key == 'k') {
		if (maxRawDepth > 1) {
			maxRawDepth -= 0.25;
		}
		else if (maxRawDepth > 0.05) {
			maxRawDepth -= 0.05;
		}
	};

	// Increase Decrease stepSize 
	if (key == 'm') stepSize += 1;
	if (key == 'n') {
		if (stepSize > 1)
			stepSize -= 1;
	};


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
