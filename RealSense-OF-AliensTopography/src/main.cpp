#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	const auto appWidth = 848 * 2;
	const auto appHeight = 480 * 2;
	ofSetupOpenGL(appWidth, appHeight, OF_WINDOW);
	ofRunApp(new ofApp());
}
