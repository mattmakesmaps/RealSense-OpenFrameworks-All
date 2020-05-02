#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	rs2_pipe.start();
	
	ofSetVerticalSync(true);
	ofBackgroundHex(0xfdefc2);

	box2d.init();
    box2d.enableEvents();   // <-- turn on the event listener
	box2d.setGravity(0, 10);
	box2d.createGround();
	box2d.setFPS(60.0);
	box2d.registerGrabbing();
	
	// register the listener so that we get the events
	ofAddListener(box2d.contactStartEvents, this, &ofApp::contactStart);
	ofAddListener(box2d.contactEndEvents, this, &ofApp::contactEnd);

	// load the 8 sfx soundfile
	for (int i=0; i<N_SOUNDS; i++) {
		sound[i].load("sfx/"+ofToString(i)+".mp3");
		sound[i].setMultiPlay(true);
		sound[i].setLoop(false);
	}
}


//--------------------------------------------------------------
void ofApp::contactStart(ofxBox2dContactArgs &e) {
	if(e.a != NULL && e.b != NULL) { 
		
		// if we collide with the ground we do not
		// want to play a sound. this is how you do that
		if(e.a->GetType() == b2Shape::e_circle && e.b->GetType() == b2Shape::e_circle) {
			
			SoundData * aData = (SoundData*)e.a->GetBody()->GetUserData();
			SoundData * bData = (SoundData*)e.b->GetBody()->GetUserData();
			
			if(aData) {
				aData->bHit = true;
				sound[aData->soundID].play();
			}
			
			if(bData) {
				bData->bHit = true;
				sound[bData->soundID].play();
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::contactEnd(ofxBox2dContactArgs &e) {
	if(e.a != NULL && e.b != NULL) { 
		
		SoundData * aData = (SoundData*)e.a->GetBody()->GetUserData();
		SoundData * bData = (SoundData*)e.b->GetBody()->GetUserData();
		
		if(aData) {
			aData->bHit = false;
		}
		
		if(bData) {
			bData->bHit = false;
		}
	}
}



//--------------------------------------------------------------
void ofApp::update() {
	
	box2d.update();
	
	// add some circles every so often
	if((int)ofRandom(0, 20) == 0) {
		
        auto c = std::make_shared<ofxBox2dCircle>();
        c->setPhysics(1, 0.5, 0.9);
        c->setup(box2d.getWorld(), (ofGetWidth()/2)+ofRandom(-30, 30), -20, ofRandom(20, 50));

        c->setData(new SoundData());
        auto * sd = (SoundData*)c->getData();
        sd->soundID = ofRandom(0, N_SOUNDS);
        sd->bHit    = false;
        
        circles.push_back(c);
	}


	// Block program until frames arrive
	rs2::frameset frames = rs2_pipe.wait_for_frames();
	// Try to get a frame of a depth image
	rs2::depth_frame depth = frames.get_depth_frame();

	auto avg_dist_mapped = ofApp::calculateDepth(depth);

	for (auto& circle : circles) {
		circle->setRadius(avg_dist_mapped);
	}

}


//--------------------------------------------------------------
void ofApp::draw() {
	
	
	for(size_t i=0; i<circles.size(); i++) {
		ofFill();
		SoundData * data = (SoundData*)circles[i].get()->getData();
		
		if(data && data->bHit) ofSetHexColor(0xff0000);
		else ofSetHexColor(0x4ccae9);
		

		circles[i].get()->draw();
	}
	
	auto ds = std::make_unique<DepthSquare>(400, 400, 40);
	ds->setDepth(avg_dist);
	ds->draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if(key == 't') ofToggleFullscreen();
    if(key == '1') box2d.enableEvents();
    if(key == '2') box2d.disableEvents();
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ) {
	
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    
    auto c = std::make_shared<ofxBox2dCircle>();
    c->setPhysics(1, 0.5, 0.9);
    c->setup(box2d.getWorld(), x, y, ofRandom(20, 50));
    
    c->setData(new SoundData());
    auto * sd = (SoundData*)c->getData();
    sd->soundID = ofRandom(0, N_SOUNDS);
    sd->bHit	= false;
    
    circles.push_back(c);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::resized(int w, int h){
}

float ofApp::calculateDepth(const rs2::depth_frame& depthFrame)
{
	/*
	Camera Update Code
	*/
	const auto rows = 10;
	const auto cols = 10;
	const auto num_cells = rows * cols;
	std::vector<float> distances;

	// Get the depth frame's dimensions
	float width = depthFrame.get_width();
	float height = depthFrame.get_height();

	const auto window_corner_x = (width / 2) - (1 / 2 * cols);
	const auto window_corner_y = (height / 2) - (1 / 2 * rows);


	for (auto i = 0; i < rows; i++)
	{
		for (auto j = 0; j < cols; j++)
		{
			auto sample_cell_x = window_corner_x + j;
			auto sample_cell_y = window_corner_y + i;
			float dist_at_cell = depthFrame.get_distance(sample_cell_x, sample_cell_y);
			distances.push_back(dist_at_cell);
		}
	}

	// filter out zeros using `Erase-remove idiom`
	distances.erase(std::remove_if(distances.begin(), distances.end(), [](float distance) {return distance < 0.1; }), distances.end());
	float sum = 0;
	for (auto& distance_sample : distances) {
		sum += distance_sample;
	}
	auto average_distance = sum / distances.size();
	avg_dist = average_distance;
	if (std::isnan(avg_dist))
		avg_dist = 0.05;
	if (std::isinf(avg_dist))
		avg_dist = 4.0;
	avg_dist_mapped = ofMap(avg_dist, 0.1, 0.6, 20, 200, true);

	/*
	End Camera Code Update
	*/
	
	string info = "";
	info += "AVG Distance: " + ofToString(avg_dist) + "\n";
	info += "AVG Mapped (Pixels): " + ofToString(avg_dist_mapped) + "\n";
	ofSetHexColor(0x444342);
	ofDrawBitmapString(info, 30, 30);

	return avg_dist_mapped;
}

