/***********************************************************************
ofApp.cpp - main openframeworks app
Copyright (c) 2016-2017 Thomas Wolf and Rasmus R. Paulsen (people.compute.dtu.dk/rapa)

This file is part of the Magic Sand.

The Magic Sand is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Magic Sand is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Augmented Reality Sandbox; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "ofApp.h"

void ofApp::setup() {
	// OF basics
	ofSetFrameRate(60);
	ofBackground(0);
	ofSetVerticalSync(true);
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetLogLevel("ofThread", OF_LOG_WARNING);
	ofSetLogLevel("ofFbo", OF_LOG_ERROR);
	ofSetLogLevel("ofShader", OF_LOG_ERROR);
	ofSetLogLevel("ofxKinect", OF_LOG_WARNING);

	// Setup rs2Projector
	rs2Projector = std::make_shared<Rs2Projector>(projWindow);
	rs2Projector->setup(true);
	
	// Setup sandSurfaceRenderer
	sandSurfaceRenderer = new SandSurfaceRenderer(rs2Projector, projWindow);
	sandSurfaceRenderer->setup(true);
	
	// Retrieve variables
	ofVec2f rs2Res = rs2Projector->getRs2Res();
	ofVec2f projRes = ofVec2f(projWindow->getWidth(), projWindow->getHeight());
	ofRectangle rs2ROI = rs2Projector->getRs2ROI();
	mainWindowROI = ofRectangle((ofGetWindowWidth()-rs2Res.x)/2, (ofGetWindowHeight()-rs2Res.y)/2, rs2Res.x, rs2Res.y);

	mapGameController.setup(rs2Projector);
	mapGameController.setProjectorRes(projRes);
	mapGameController.setRs2Res(rs2Res);
	mapGameController.setRs2ROI(rs2ROI);

	boidGameController.setup(rs2Projector);
	boidGameController.setProjectorRes(projRes);
	boidGameController.setRs2Res(rs2Res);
	boidGameController.setRs2ROI(rs2ROI);

}


void ofApp::update() {
    // Call rs2Projector->update() first during the update function()
	rs2Projector->update();
   	sandSurfaceRenderer->update();
    
	if (rs2Projector->getRs2ROI() != mapGameController.getRs2ROI())
	{
		ofRectangle rs2ROI = rs2Projector->getRs2ROI();
		mapGameController.setRs2ROI(rs2ROI);
		boidGameController.setRs2ROI(rs2ROI);
	}

	mapGameController.update();
	boidGameController.update();
}


void ofApp::draw() 
{
	float x = mainWindowROI.x;
	float y = mainWindowROI.y;
	float w = mainWindowROI.width;
	float h = mainWindowROI.height;

	if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING)
	{
		sandSurfaceRenderer->drawMainWindow(x, y, w, h);//400, 20, 400, 300);
		boidGameController.drawMainWindow(x, y, w, h);
	}

	rs2Projector->drawMainWindow(x, y, w, h);
}

void ofApp::drawProjWindow(ofEventArgs &args) 
{
	if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING)
	{
		sandSurfaceRenderer->drawProjectorWindow();
		mapGameController.drawProjectorWindow();
		boidGameController.drawProjectorWindow();
	}
	rs2Projector->drawProjectorWindow();
}

void ofApp::keyPressed(int key) 
{
	if (key == 'c')
	{
		rs2Projector->SaveRs2ColorImage();
	}
	else if (key == 'd')
	{
		rs2Projector->SaveFilteredDepthImage();
	}
	else if (key == ' ')
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING &&
			boidGameController.isIdle()) // do not start map game if boidgame is not idle
		{
			if (mapGameController.isIdle())
			{
				mapGameController.setDebug(rs2Projector->getDumpDebugFiles());
				mapGameController.StartGame();
			}
			else
			{
				mapGameController.ButtonPressed();
			}
		}
		else if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_SETUP)
		{
			// Try to start the application
			rs2Projector->startApplication();
		}
	}
	else if (key == 'f' || key == 'r')
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING)
		{
			if (mapGameController.isIdle())
			{
				boidGameController.setDebug(rs2Projector->getDumpDebugFiles());
				boidGameController.StartGame(2);
			}
			else 
			{
				mapGameController.EndButtonPressed();
			}
		}
	}
	else if (key == '1') // Absolute beginner
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING && mapGameController.isIdle())
		{
			boidGameController.setDebug(rs2Projector->getDumpDebugFiles());
			boidGameController.StartGame(0);
		}
	}
	else if (key == '2') 
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING && mapGameController.isIdle())
		{
			boidGameController.setDebug(rs2Projector->getDumpDebugFiles());
			boidGameController.StartGame(1);
		}
	}
	else if (key == '3')
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING && mapGameController.isIdle())
		{
			boidGameController.setDebug(rs2Projector->getDumpDebugFiles());
			boidGameController.StartGame(2);
		}
	}
	else if (key == '4')
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING && mapGameController.isIdle())
		{
			boidGameController.setDebug(rs2Projector->getDumpDebugFiles());
			boidGameController.StartGame(3);
		}
	}
	else if (key == 'm')
	{
		if (rs2Projector->GetApplicationState() == Rs2Projector::APPLICATION_STATE_RUNNING && mapGameController.isIdle())
		{
			boidGameController.setDebug(rs2Projector->getDumpDebugFiles());
			boidGameController.StartSeekMotherGame();
		}
	}
	else if (key == 't')
	{
		mapGameController.setDebug(rs2Projector->getDumpDebugFiles());
		mapGameController.RealTimeTestMe();
	}
	else if (key == 'w')
	{
		mapGameController.setDebug(rs2Projector->getDumpDebugFiles());
		mapGameController.DebugTestMe();
	}
}

void ofApp::keyReleased(int key) {

}

void ofApp::mouseMoved(int x, int y) {

}

void ofApp::mouseDragged(int x, int y, int button) {

	// We assume that we only use this during ROI annotation
	rs2Projector->mouseDragged(x - mainWindowROI.x, y - mainWindowROI.y, button);
}

void ofApp::mousePressed(int x, int y, int button) 
{
	if (mainWindowROI.inside((float)x, (float)y))
	{
		rs2Projector->mousePressed(x-mainWindowROI.x, y-mainWindowROI.y, button);
	}
}

void ofApp::mouseReleased(int x, int y, int button) {
	// We assume that we only use this during ROI annotation
	rs2Projector->mouseReleased(x - mainWindowROI.x, y - mainWindowROI.y, button);

}

void ofApp::mouseEntered(int x, int y) {

}

void ofApp::mouseExited(int x, int y) {

}

void ofApp::windowResized(int w, int h) {

}

void ofApp::gotMessage(ofMessage msg) {

}

void ofApp::dragEvent(ofDragInfo dragInfo) {

}

