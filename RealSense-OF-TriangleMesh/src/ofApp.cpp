#include "ofApp.h"
#include <string>
#include <iostream>

const int ofApp::depthFrameWidth = 840;
const int ofApp::depthFrameHeight = 480;
const int ofApp::appWidth = depthFrameWidth * 2;
const int ofApp::appHeight = depthFrameHeight * 2;

namespace {
	bool enableNoiseSmoothing = true;
	bool labelPoints = false;
	auto minRawDepth = 0.1;
	auto maxRawDepth = 2.0;
	auto minMappedDepth = 1;
	auto maxMappedDepth = 1000;
	auto spotZ = 100;
	auto spotX = 100;
	auto spotY = -175;
	int stepSize = 7;

	typedef std::pair <std::string, ofPrimitiveMode> primativePair;
	vector<primativePair> primativeModes = {
		primativePair("OF_PRIMITIVE_LINE_STRIP",OF_PRIMITIVE_LINE_STRIP),
		primativePair("OF_PRIMITIVE_LINE_LOOP",OF_PRIMITIVE_LINE_LOOP),
		primativePair("OF_PRIMITIVE_LINES",OF_PRIMITIVE_LINES),
		primativePair("OF_PRIMITIVE_POINTS",OF_PRIMITIVE_POINTS),
		primativePair("OF_PRIMITIVE_TRIANGLES",OF_PRIMITIVE_TRIANGLES),
		primativePair("OF_PRIMITIVE_TRIANGLE_STRIP",OF_PRIMITIVE_TRIANGLE_STRIP),
		primativePair("OF_PRIMITIVE_TRIANGLE_FAN",OF_PRIMITIVE_TRIANGLE_FAN)
	};

	vector<primativePair>::iterator primativeModeIterator = primativeModes.begin();
}

//--------------------------------------------------------------
void ofApp::setup() {
	rs2_pipe.start();
	ofSetVerticalSync(true);

	spot.setup();

	meshMaterial.setDiffuseColor(ofColor::orange);
	meshMaterial.setShininess(0.01);

	glEnable(GL_POINT_SMOOTH); // use circular points instead of square points
	glPointSize(3); // make the points bigger

}

//--------------------------------------------------------------
void ofApp::update() {

	// Block program until frames arrive
	rs2::frameset frames = rs2_pipe.wait_for_frames();
	// Try to get a frame of a depth image
	rs2::depth_frame depth = frames.get_depth_frame();

	int vertCounter = 0;
	mesh.clear();
	mesh.setMode(primativeModeIterator->second);
	mesh.enableIndices();

	spot.setPosition(spotX,spotY,spotZ);

	// loop through the image in the x and y axes
	for (int y = 0; y < depthFrameHeight; y += stepSize) {


		for (int x = 0; x < depthFrameWidth; x += stepSize) {

			// map depthValue to extrude it a bit
			auto depthValue = depth.get_distance(x, y);
			auto extrudedDepthValue = ofMap(depthValue, minRawDepth, maxRawDepth, minMappedDepth, maxMappedDepth, false);

			// arbitrarilly set outlier point to `minMappedDepth - 1` as a signal it needs to be interpolated.
			if (enableNoiseSmoothing && (extrudedDepthValue < minMappedDepth || extrudedDepthValue > maxMappedDepth))
			{
				extrudedDepthValue = minMappedDepth - 1; // -1 to bypass any weird float comparision.
			}

			glm::vec3 pos(x, y, extrudedDepthValue);
			mesh.addVertex(pos);
			mesh.addIndex(vertCounter);
			vertCounter++;
		}

		// Iterate through the completed mesh and interpolate if needed.
		if (enableNoiseSmoothing)
		{
			for (int i = 0; i < mesh.getNumVertices(); i++)
			{
				auto targetVert = mesh.getVertex(i);
				bool targetVertDirty = false;
				bool hasPrevVert = (i != 0 ? true : false);
				bool hasNextVert = (i != (mesh.getNumVertices() - 1) ? true : false);

				// lerp interior nodes that are outliers
				if (targetVert.z < minMappedDepth && (hasPrevVert && hasNextVert)) {
					targetVertDirty = true;
					auto prevVertZ = mesh.getVertex(i - 1).z;
					auto nextVertZ = mesh.getVertex(i + 1).z;
					targetVert.z = ofLerp(prevVertZ, nextVertZ, 0.5);
				}
				
				// set exterior nodes that our outliers to maxMappedDepth
				if (hasPrevVert != hasNextVert) {
					targetVertDirty = true;
					targetVert.z = maxMappedDepth;
				}

				if (targetVertDirty)
					mesh.setVertex(i, targetVert);
			}
		}

	}

	// MK TODO: This needs to take into account the step size
	// Add indexes for triangle strip primative
	for (int y = 0; y < (depthFrameHeight / stepSize); y++) {
		for (int x = 0; x < (depthFrameWidth / stepSize); x++) {
			mesh.addIndex(x + y * (depthFrameWidth / stepSize));               // 0
			mesh.addIndex((x + 1) + y * (depthFrameWidth / stepSize));           // 1
			mesh.addIndex(x + (y + 1) * (depthFrameWidth / stepSize));           // 10

			mesh.addIndex((x + 1) + y * (depthFrameWidth / stepSize));           // 1
			mesh.addIndex((x + 1) + (y + 1) * (depthFrameWidth / stepSize));       // 11
			mesh.addIndex(x + (y + 1) * (depthFrameWidth / stepSize));           // 10
		}
	}

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofEnableDepthTest();
	ofBackgroundGradient(ofColor::black, ofColor::black, OF_GRADIENT_CIRCULAR);
	
	auto vertCount = mesh.getNumVertices();
	auto centerIshNode = mesh.getVertex(vertCount / 2);

	// even points can overlap with each other, let's avoid that
	spot.enable();
	cam.begin();

	ofRotateZDeg(180);
	ofRotateXDeg(270);
	ofTranslate(-appWidth / 4 , 0, -appHeight/4);

	meshMaterial.begin();
	mesh.draw();
	meshMaterial.end();
	if (labelPoints)
	{
		for (int i = 0; i < mesh.getNumVertices(); i++)
		{
			auto vertX = mesh.getVertex(i).x;
			auto vertY = mesh.getVertex(i).y;
			auto vertZ = mesh.getVertex(i).z;
			stringstream sPos;

			/* uncomment for format: <point:x,y,z> */
			sPos << i << ":" << vertX << "," << vertY << "," << vertZ << std::endl;
			ofDrawBitmapString(sPos.str().c_str(), vertX + 1, vertY + 1, vertZ + 1);

			/* uncomment for format: <point> */
			/*
			sPos << i << std::endl;
			ofDrawBitmapString(sPos.str().c_str(), vertX + 1, vertY + 1, vertZ + 1);
			*/
		}
	}
	cam.end();

	ofDisableDepthTest();
	// Draw Text
	stringstream ss;
	ss << "Point Density (m, n): " << stepSize << std::endl;
	ss << "minRawDepth (p,o): " << minRawDepth << std::endl;
	ss << "maxnRawDepth (l,k): " << maxRawDepth << std::endl;
	ss << "enableNoiseSmoothing (f): " << (enableNoiseSmoothing ? "true" : "false") << std::endl;
	ss << "labelPoints (u): " << (labelPoints ? "true" : "false") << std::endl;
	ss << "primativeMode (y): " << primativeModeIterator->first << std::endl;
	ss << "spotZ (q, w): " << spotZ << std::endl;
	ss << "spotX (a, s): " << spotX << std::endl;
	ss << "spotY (z, x): " << spotY << std::endl;
	ofDrawBitmapStringHighlight(ss.str().c_str(), 20, 20, ofColor::white, ofColor::black);

	spot.disable();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	// Toggle Filtering
	if (key == 'f')
		enableNoiseSmoothing = !enableNoiseSmoothing;

	// Label Points
	if (key == 'u')
		labelPoints = !labelPoints;

	// Cycle Primative Mode 
	if (key == 'y') {
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

	// Increase Decrease spotZ 
	if (key == 'w')
		spotZ += 5;
	if (key == 'q')
		spotZ -= 5;

	// Increase Decrease spotX
	if (key == 's')
		spotX += 5;
	if (key == 'a')
		spotX -= 5;

	// Increase Decrease spotY
	if (key == 'x')
		spotY += 5;
	if (key == 'z')
		spotY -= 5;

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
