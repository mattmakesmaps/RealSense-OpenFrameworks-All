#include "ofApp.h"
#include <string>
#include <iostream>

const int ofApp::depthFrameWidth = 848;
const int ofApp::depthFrameHeight = 480;
const int ofApp::appWidth = depthFrameWidth * 2;
const int ofApp::appHeight = depthFrameHeight * 2;
int ofApp::squareLength = 60;

namespace {
	bool filterNoise = false;
	bool connectLines = false;
	auto connectDistance = 50;
	auto minRawDepth = 0.1;
	auto maxRawDepth = 5.0;
	auto minMappedDepth = 1;
	auto maxMappedDepth = 1000;
	int stepSize = 4;

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
	if (connectLines)
	{
		mesh.setMode(primativeModeIterator->second);
		mesh.enableIndices();
	}
	else {
		mesh.setMode(OF_PRIMITIVE_POINTS);
	}

	mesh.clear();

	// Block program until frames arrive
	rs2::frameset frames = rs2_pipe.wait_for_frames();
	// Try to get a frame of a depth image
	rs2::depth_frame depth = frames.get_depth_frame();

	// loop through the image in the x and y axes
	for (int y = 0; y < depthFrameHeight; y += stepSize) {
		for (int x = 0; x < depthFrameWidth; x += stepSize) {
			const ofColor pointColor = ofColor::yellow;
			auto depthValue = depth.get_distance(x, y);

			// map depthValue to extrude it a bit
			auto extrudedDepthValue = ofMap(depthValue, minRawDepth, maxRawDepth, minMappedDepth, maxMappedDepth, true);
			mesh.addColor(pointColor);
			glm::vec3 pos(x, y, extrudedDepthValue);
			// ignore floor/ceiling points
			if (filterNoise && (extrudedDepthValue > minMappedDepth && extrudedDepthValue < maxMappedDepth)) {
				mesh.addVertex(pos);
			}
			else if (!filterNoise)
			{
				mesh.addVertex(pos);
			}
		}
	}


	// https://openframeworks.cc/ofBook/chapters/generativemesh.html
	if (connectLines)
	{
		int numVerts = mesh.getNumVertices();
		for (int a = 0; a < numVerts; ++a) {
			ofVec3f verta = mesh.getVertex(a);
			for (int b = a + 1; b < numVerts; ++b) {
				ofVec3f vertb = mesh.getVertex(b);
				float distance = verta.distance(vertb);
				if (distance <= connectDistance) {
					// In OF_PRIMITIVE_LINES, every pair of vertices or indices will be
					// connected to form a line
					mesh.addIndex(a);
					mesh.addIndex(b);
				}
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackgroundGradient(ofColor::gray, ofColor::black, OF_GRADIENT_CIRCULAR);

	// even points can overlap with each other, let's avoid that
	cam.begin();
	ofScale(2, -2, 2); // flip the y axis and zoom in a bit
	ofRotateYDeg(90);
	ofTranslate(-appWidth / 2, -appHeight / 2);
	mesh.draw();
	//mesh.drawFaces();
	//mesh.drawWireframe();
	cam.end();

	// Draw Text
	stringstream ss;
	ss << "Point Density (m, n): " << stepSize << std::endl;
	ss << "minRawDepth (p,o): " << minRawDepth << std::endl;
	ss << "maxnRawDepth (l,k): " << maxRawDepth << std::endl;
	ss << "filterNoise (f): " << (filterNoise ? "true" : "false") << std::endl;
	ss << "connectLines (c): " << (connectLines ? "true" : "false") << std::endl;
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

	// Toggle connectLines (populates indicies)
	if (key == 'c')
		connectLines = !connectLines;

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

	// Increase Decrease connectDistance
	if (key == 't') connectDistance += 5;
	if (key == 'r') {
		if (connectDistance > 5)
			connectDistance -= 5;
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
