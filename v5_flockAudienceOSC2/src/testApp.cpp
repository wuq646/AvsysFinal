#include "testApp.h"


// comparison routine for sort...
bool comparisonFunction(  particle * a, particle * b ) {
	return a->pos.x < b->pos.x;
}

//--------------------------------------------------------------
void testApp::setup(){	
	
    receiver.setup(PORT);
	
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
    ofEnableAlphaBlending();
	
    rgbaFbo.allocate(ofGetScreenWidth(), ofGetScreenHeight(), GL_RGBA);
    rgbaFboFloat.allocate(ofGetScreenWidth(), ofGetScreenHeight(), GL_RGBA32F_ARB);
	
    rgbaFbo.begin();
	ofClear(255,255,255, 0);
    rgbaFbo.end();
    
    rgbaFboFloat.begin();
	ofClear(255,255,255, 0);
    rgbaFboFloat.end();
    
    fadeAmnt=10;
    
    
	for (int i = 0; i < PARTICLE_NUM; i++){
		particle * myParticle = new particle;
        myParticle->setInitialCondition(ofRandom(0,ofGetWidth()),ofRandom(0,ofGetHeight()),0,0);
		particles.push_back(myParticle);
	}
	
    
	panel.setup("control", 770, 0, 300, 400);
	panel.addPanel("flocking", 1, false);
	panel.setWhichPanel("flocking");
	panel.setWhichColumn(0);
	panel.addSlider("cohesion strength", "COHESION_STRENGTH", 0.01,0,1, false);
	panel.addSlider("cohesion radius", "COHESION_RADIUS", 50,0,200, false);
	panel.addSlider("alignment strength", "ALIGNMENT_STRENGTH", 0.01,0,1, false);
	panel.addSlider("alignment radius", "ALIGNMENT_RADIUS",  50,0,200, false);
	panel.addSlider("seperation strength", "SEPERATION_STRENGTH", 0.01,0, 1, false);
	panel.addSlider("seperation radius", "SEPERATION_RADIUS",  50,0,200, false);
	
	panel.addSlider("damping", "DAMPING", 0.01, 0.001, 0.5, false);
	
	panel.loadSettings("settings.xml");
}

//--------------------------------------------------------------
void testApp::update(){

    
	panel.update();
	
    
    rgbaFboFloat.begin();
    drawFboTest();
	rgbaFboFloat.end();
    
    
    // sort all the particles:
	sort( particles.begin(), particles.end(), comparisonFunction );
	
    for (int i = 0; i < particles.size(); i++){
		particles[i]->cohesion.distance = panel.getValueF("COHESION_RADIUS");
		particles[i]->alignment.distance = panel.getValueF("ALIGNMENT_RADIUS");
		particles[i]->seperation.distance = panel.getValueF("SEPERATION_RADIUS");
        particles[i]->cohesion.strength = panel.getValueF("COHESION_STRENGTH");
		particles[i]->alignment.strength = panel.getValueF("ALIGNMENT_STRENGTH");
		particles[i]->seperation.strength = panel.getValueF("SEPERATION_STRENGTH");
		particles[i]->damping = panel.getValueF("DAMPING");

        
	}
	
	for (int i = 0; i < particles.size(); i++){
		particles[i]->resetForce();
	}

	
    for (int i = 0; i < particles.size(); i++){
		for (int j = i-1; j >= 0; j--){
			if ( fabs(particles[j]->pos.x - particles[i]->pos.x) >	50) break;
                if (i != j){
                    particles[i]->addForFlocking(*particles[j]);
                }
		}
        particles[i]->addRepulsionForce(mouseX, mouseY, 40, 0.4);
	}
    
	for (int i = 0; i < particles.size(); i++){
		particles[i]->addFlockingForce();
		particles[i]->addDampingForce();
		particles[i]->update();
	}
	
	
	// wrap torroidally.
	for (int i = 0; i < particles.size(); i++){
		ofVec2f pos = particles[i]->pos;
		if (pos.x < 0) pos.x = ofGetWidth();
		if (pos.x > ofGetWidth()) pos.x = 0;
		if (pos.y < 0) pos.y = ofGetHeight();
		if (pos.y > ofGetHeight()) pos.y = 0;
		particles[i]->pos = pos;
	}

    //osc
    
    // hide old messages
	for(int i = 0; i < NUM_MSG_STRINGS; i++){
		if(timers[i] < ofGetElapsedTimef()){
			msg_strings[i] = "";
		}
	}
    
	// check for waiting messages
	while(receiver.hasWaitingMessages()){
		// get the next message
		ofxOscMessage m;
		receiver.getNextMessage(&m);
        for (int i = 0; i < particles.size(); i++){
            // check for mouse moved message
            if(m.getAddress() == "/accel/x"){
                particles[i]->r3 = m.getArgAsFloat(0);
                particles[i]->vel.x += m.getArgAsFloat(0);
                cout << m.getArgAsFloat(0) << endl;
            }
            
            if(m.getAddress() == "/accel/y"){
                particles[i]->r2 = m.getArgAsFloat(0);
                particles[i]->vel.y += m.getArgAsFloat(0);
            }
            
            if(m.getAddress() == "/accel/z"){
                particles[i]->r1 = m.getArgAsFloat(0);
                particles[i]->vel.z += m.getArgAsFloat(0);
            }

            
            if(m.getAddress() == "/hue"){
                particles[i]->c.setHue(m.getArgAsFloat(0));
            }
            
            if(m.getAddress() == "/sat"){
                particles[i]->c.setSaturation(m.getArgAsFloat(0));
            }
            
            
            if(m.getAddress() == "/midi/cc2/2"){
                // both the arguments are int32's
                Vbase = m.getArgAsFloat(0);
                particles[i]->cohesion.distance= Vbase*100;
            }
            
            if(m.getAddress() == "/midi/cc3/2"){
                // both the arguments are int32's
                Vmelody = m.getArgAsFloat(0);
                particles[i]->alignment.strength = Vmelody;
            }
            
            if(m.getAddress() == "/midi/cc1/1"){
                // both the arguments are int32's
                Edrum = m.getArgAsFloat(0);
                particles[i]->alignment.distance = Edrum*100;
            }
            
            if(m.getAddress() == "/midi/cc2/1"){
                // both the arguments are int32's
                Ebase = m.getArgAsFloat(0);
                particles[i]->seperation.strength = Ebase;
            }
            
            if(m.getAddress() == "/midi/cc3/1"){
                // both the arguments are int32's
                Emelody = m.getArgAsFloat(0);
                particles[i]->seperation.distance = Emelody*100;
            }
            
            else{
                // unrecognized message: display on the bottom of the screen
                string msg_string;
                msg_string = m.getAddress();
                msg_string += ": ";
                for(int i = 0; i < m.getNumArgs(); i++){
                    // get he argument type
                    msg_string += m.getArgTypeName(i);
                    msg_string += ":";
                    // display the argument - make sure we get the right type
                    if(m.getArgType(i) == OFXOSC_TYPE_INT32){
                        msg_string += ofToString(m.getArgAsInt32(i));
                    }
                    else if(m.getArgType(i) == OFXOSC_TYPE_FLOAT){
                        msg_string += ofToString(m.getArgAsFloat(i));
                    }
                    else if(m.getArgType(i) == OFXOSC_TYPE_STRING){
                        msg_string += m.getArgAsString(i);
                    }
                    else{
                        msg_string += "unknown";
                    }
                }
                // add to the list of strings to display
                msg_strings[current_msg_string] = msg_string;
                timers[current_msg_string] = ofGetElapsedTimef() + 5.0f;
                current_msg_string = (current_msg_string + 1) % NUM_MSG_STRINGS;
                // clear the next line
                msg_strings[current_msg_string] = "";
            }
            
        }
        
	}
    
    
    
    

}

//--------------------------------------------------------------
void testApp::draw(){
	
    
    ofSetColor(255,255,255);
    rgbaFbo.draw(0,0);
    rgbaFboFloat.draw(0,0);
    
    
//    ofDrawBitmapString("non floating point FBO", ofPoint(10,20));
//    ofDrawBitmapString("floating point FBO", ofPoint(420,20));
//    
//	string alphaInfo = "Current alpha fade amnt = " + ofToString(fadeAmnt);
//	alphaInfo += "\nHold '1' to set alpha fade to 1";
//	alphaInfo += "\nHold '2' to set alpha fade to 5";
//	alphaInfo += "\nHold '3' to set alpha fade to 15";
//	alphaInfo += "\nHold 'c' to clear the fbo each frame\n\nMove mouse to draw with a circle";
//	
//    ofDrawBitmapString(alphaInfo, ofPoint(10,430));
    
    
    
    
	panel.draw();
   
    for (int i = 0; i < particles.size(); i++){
    
    ofDrawBitmapStringHighlight("Vbase " + ofToString(Vbase)
                                + "\nVmelody " + ofToString(Vmelody)
                                + "\nEdrum " + ofToString(Edrum)
                                + "\nEbase " + ofToString(Ebase)
                                + "\nEmelody " + ofToString(Emelody)
                                + "\n/r3" + ofToString(particles[i]->r3)
                                + "\n/r2" +  ofToString(particles[i]->r2)
                                + "\n/r1" +  ofToString(particles[i]->r1)
                                + "\n/VelX" + ofToString(particles[i]->vel.x)
                                + "\n/VelY" +  ofToString(particles[i]->vel.y)
                                + "\n/VelZ" +  ofToString(particles[i]->vel.z),10,10);
    Vbase, Vmelody, Edrum, Ebase, Emelody;
    
    }
}
//--------------------------------------------------------------
void testApp::drawFboTest(){
	//we clear the fbo if c is pressed.
	//this completely clears the buffer so you won't see any trails
	if( ofGetKeyPressed('c') ){
		ofClear(255,255,255, 0);
	}
	
	//some different alpha values for fading the fbo
	//the lower the number, the longer the trails will take to fade away.
//	fadeAmnt = 40;
//	if(ofGetKeyPressed('1')){
//		fadeAmnt = 1;
//	}else if(ofGetKeyPressed('2')){
//		fadeAmnt = 5;
//	}else if(ofGetKeyPressed('3')){
//		fadeAmnt = 15;
//	}
    
	//1 - Fade Fbo
	
	//this is where we fade the fbo
	//by drawing a rectangle the size of the fbo with a small alpha value, we can slowly fade the current contents of the fbo.
	ofFill();
    
    ofColor dark(40,fadeAmnt);
    ofColor black(0, fadeAmnt);
	ofBackgroundGradient(dark, black);
    
//	ofSetColor(255,255,255, fadeAmnt);
//	ofRect(0,0,ofGetScreenWidth(),ofGetScreenHeight());
    
	//2 - Draw graphics
	
	
	for (int i = 0; i < particles.size(); i++){
		particles[i]->draw();
	}
    
    
}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	switch (key){
			
		case ' ':
			// reposition everything: 
			for (int i = 0; i < particles.size(); i++){
				particles[i]->setInitialCondition(ofRandom(0,ofGetWidth()),ofRandom(0,ofGetHeight()),0,0);
			}
			break;
        case 'f': ofToggleFullscreen();
	}
	
	
}

//--------------------------------------------------------------
void testApp::keyReleased  (int key){ 
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
//	if (x < 500){
//		particles.erase(particles.begin());
//		particle * myParticle;
//		float randomAngle = ofRandom(0, TWO_PI);
//		myParticle->setInitialCondition(x,y,cos(randomAngle),sin(randomAngle));
//		particles.push_back(myParticle);
//	}
    
    
    
	panel.mouseDragged(x,y,button);
    
    fadeAmnt = ofMap(x, 0, ofGetWidth(),0, 40);
	
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	
    particles.erase(particles.begin());
    
    
	panel.mousePressed(x,y,button);
	
}

//--------------------------------------------------------------
void testApp::mouseReleased(){
	
	panel.mouseReleased();
}
