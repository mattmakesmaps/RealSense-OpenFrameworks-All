#pragma once

#include <librealsense2/rs.hpp>
#include "ofMain.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
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

		rs2::pipeline rs2_pipe;

		static const int appWidth;
		static const int appHeight;
		static const int depthFrameWidth;
		static const int depthFrameHeight;

		ofEasyCam cam;
		ofMesh mesh;
		ofLight spot;
		std::vector<std::unique_ptr<ofMesh>> meshes;
};
