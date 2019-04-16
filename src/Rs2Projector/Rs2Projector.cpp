/***********************************************************************
KinectProjector - KinectProjector takes care of the spatial conversion
between the various coordinate systems, control the kinectgrabber and
perform the calibration of the kinect and projector.
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
with the Magic Sand; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "Rs2Projector.h"
#include <sstream>

using namespace ofxCSG;

Rs2Projector::Rs2Projector(std::shared_ptr<ofAppBaseWindow> const& p)
:ROIcalibrated(false),
projRs2Calibrated(false),
//calibrating (false),
basePlaneUpdated (false),
basePlaneComputed(false),
projRs2CalibrationUpdated (false),
//ROIUpdated (false),
imageStabilized (false),
waitingForFlattenSand (false),
drawRs2View(false),
drawRs2ColorView(true)
{
	doShowROIonProjector = false;
	applicationState = APPLICATION_STATE_SETUP;
    projWindow = p;
	TemporalFilteringType = 1;
	DumpDebugFiles = true;
	DebugFileOutDir = "DebugFiles//";
}

void Rs2Projector::setup(bool sdisplayGui)
{
	applicationState = APPLICATION_STATE_SETUP;

	ofAddListener(ofEvents().exit, this, &Rs2Projector::exit);

	// instantiate the modal windows //
    modalTheme = make_shared<ofxModalThemeProjRs2>();
    confirmModal = make_shared<ofxModalConfirm>();
    confirmModal->setTheme(modalTheme);
    confirmModal->addListener(this, &Rs2Projector::onConfirmModalEvent);
    confirmModal->setButtonLabel("Ok");
    
    calibModal = make_shared<ofxModalAlert>();
    calibModal->setTheme(modalTheme);
    calibModal->addListener(this, &Rs2Projector::onCalibModalEvent);
    calibModal->setButtonLabel("Cancel");
        
	displayGui = sdisplayGui;

    // calibration chessboard config
	chessboardSize = 300;
	chessboardX = 5;
    chessboardY = 4;
    
    // 	Gradient Field
	gradFieldResolution = 10;
    arrowLength = 25;
	
    // Setup default base plane
	basePlaneNormalBack = ofVec3f(0,0,1); // This is our default baseplane normal
	basePlaneOffsetBack= ofVec3f(0,0,870); // This is our default baseplane offset
    basePlaneNormal = basePlaneNormalBack;
    basePlaneOffset = basePlaneOffsetBack;
    basePlaneEq=getPlaneEquation(basePlaneOffset,basePlaneNormal);
    maxOffsetBack = basePlaneOffset.z-300;
    maxOffset = maxOffsetBack;
    maxOffsetSafeRange = 50; // Range above the autocalib measured max offset

    // rs2grabber: start & default setup
	rs2Opened = rs2grabber.setup();
	lastRs2OpenTry = ofGetElapsedTimef();
	if (!rs2Opened)
	{
		// If the rs2 is not found and opened (which happens very often on Windows 10) then just go with default values for the Rs2
		ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): Rs2 not found - trying again later";
	}

	doInpainting = false;
	doFullFrameFiltering = false;
	spatialFiltering = true;
    followBigChanges = false;
    numAveragingSlots = 15;
	TemporalFrameCounter = 0;
    
    // Get projector and rs2 width & height
    projRes = ofVec2f(projWindow->getWidth(), projWindow->getHeight());
    rs2Res = rs2grabber.getRs2Size();
	rs2ROI = ofRectangle(0, 0, rs2Res.x, rs2Res.y);
	ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): rs2ROI " << rs2ROI;

    // Initialize the fbos and images
    FilteredDepthImage.allocate(rs2Res.x, rs2Res.y);
    rs2ColorImage.allocate(rs2Res.x, rs2Res.y);
    thresholdedImage.allocate(rs2Res.x, rs2Res.y);
    
	kpt = new ofxRs2ProjectorToolkit(projRes, rs2Res);

	// finish rs2grabber setup and start the grabber
    rs2grabber.setupFramefilter(gradFieldResolution, maxOffset, rs2ROI, spatialFiltering, followBigChanges, numAveragingSlots);
    rs2WorldMatrix = rs2grabber.getWorldMatrix();
    ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): rs2WorldMatrix: " << rs2WorldMatrix ;
    
    // Setup gradient field
    setupGradientField();
    
    // init FBOprojector
    init_FBOprojector();
    
    fboMainWindow.allocate(rs2Res.x, rs2Res.y, GL_RGBA);
    fboMainWindow.begin();
    ofClear(255, 255, 255, 0);
    fboMainWindow.end();

    if (displayGui)
        setupGui();

    rs2grabber.start(); // Start the acquisition

	updateStatusGUI();
    
    // Calibration Error count
    errorcounts = 0.0;
}

void Rs2Projector::exit(ofEventArgs& e)
{
	if (ROIcalibrated)
	{
		if (saveSettings())
		{
			ofLogVerbose("Rs2Projector") << "exit(): Settings saved ";
		}
		else {
			ofLogVerbose("Rs2Projector") << "exit(): Settings could not be saved ";
		}
	}
}

void Rs2Projector::setupGradientField(){
    gradFieldcols = rs2Res.x / gradFieldResolution;
    gradFieldrows = rs2Res.y / gradFieldResolution;
    
    gradField = new ofVec2f[gradFieldcols*gradFieldrows];
    ofVec2f* gfPtr=gradField;
    for(unsigned int y=0;y<gradFieldrows;++y)
        for(unsigned int x=0;x<gradFieldcols;++x,++gfPtr)
            *gfPtr=ofVec2f(0);
}

void Rs2Projector::setGradFieldResolution(int sgradFieldResolution){
    gradFieldResolution = sgradFieldResolution;
    setupGradientField();
    rs2grabber.performInThread([sgradFieldResolution](Rs2Grabber & kg) {
        kg.setGradFieldResolution(sgradFieldResolution);
    });
}

// For some reason this call eats milliseconds - so it should only be called when something is changed
// else it would be convenient just to call it in every update
void Rs2Projector::updateStatusGUI()
{
	if (rs2Opened)
	{
		StatusGUI->getLabel("Rs2 Status")->setLabel("Rs2 running");
		StatusGUI->getLabel("Rs2 Status")->setLabelColor(ofColor(0, 255, 0));
	}
	else
	{
		StatusGUI->getLabel("Rs2 Status")->setLabel("Rs2 not found");
		StatusGUI->getLabel("Rs2 Status")->setLabelColor(ofColor(255, 0, 0));
	}
    if(imageStabilized){
        StatusGUI->getLabel("Ready Calibration")->setLabel("ready calibration");
        StatusGUI->getLabel("Ready Calibration")->setLabelColor(ofColor(0, 255, 0));
    }else{
        StatusGUI->getLabel("Ready Calibration")->setLabel("not ready calibration");
        StatusGUI->getLabel("Ready Calibration")->setLabelColor(ofColor(255, 0, 0));
    }
	if (ROIcalibrated)
	{
		StatusGUI->getLabel("ROI Status")->setLabel("ROI defined");
		StatusGUI->getLabel("ROI Status")->setLabelColor(ofColor(0, 255, 0));
	}
	else
	{
		StatusGUI->getLabel("ROI Status")->setLabel("ROI not defined");
		StatusGUI->getLabel("ROI Status")->setLabelColor(ofColor(255, 0, 0));
	}

	if (basePlaneComputed)
	{
		StatusGUI->getLabel("Baseplane Status")->setLabel("Baseplane found");
		StatusGUI->getLabel("Baseplane Status")->setLabelColor(ofColor(0, 255, 0));
	}
	else
	{
		StatusGUI->getLabel("Baseplane Status")->setLabel("Baseplane not found");
		StatusGUI->getLabel("Baseplane Status")->setLabelColor(ofColor(255, 0, 0));
	}

	if (projRs2Calibrated)
	{
		StatusGUI->getLabel("Calibration Status")->setLabel("Projector/Rs2 calibrated");
		StatusGUI->getLabel("Calibration Status")->setLabelColor(ofColor(0, 255, 0));
	}
	else
	{
		StatusGUI->getLabel("Calibration Status")->setLabel("Projector/Rs2 not calibrated");
		StatusGUI->getLabel("Calibration Status")->setLabelColor(ofColor(255, 0, 0));
	}
    
    StatusGUI->getLabel("Calibration Error Count")->setLabel("Calibration Error count : " + std::to_string((int)errorcounts));
	StatusGUI->getLabel("Projector Status")->setLabel("Projector " + ofToString(projRes.x) + " x " + ofToString(projRes.y));

	std::string AppStatus = "Setup";
	if (applicationState == APPLICATION_STATE_CALIBRATING)
		AppStatus = "Calibrating";
	else if (applicationState == APPLICATION_STATE_RUNNING)
		AppStatus = "Running";

	StatusGUI->getLabel("Application Status")->setLabel("Application state: " + AppStatus);
	StatusGUI->getLabel("Application Status")->setLabelColor(ofColor(255, 255, 0));

	StatusGUI->getLabel("Calibration Step")->setLabel("Calibration Step: " + calibrationText);;
	StatusGUI->getLabel("Calibration Step")->setLabelColor(ofColor(0, 255, 255));

	gui->getToggle("Spatial filtering")->setChecked(spatialFiltering);
	gui->getToggle("Quick reaction")->setChecked(followBigChanges);
	gui->getToggle("Inpaint outliers")->setChecked(doInpainting);
	gui->getToggle("Full Frame Filtering")->setChecked(doFullFrameFiltering);
}

void Rs2Projector::update()
{
    updateStatusGUI();
    // Clear updated state variables
    basePlaneUpdated = false;
    projRs2CalibrationUpdated = false;

	// Try to open the rs2 every 3. second if it is not yet open
	float TimeStamp = ofGetElapsedTimef();
	if (!rs2Opened && TimeStamp-lastRs2OpenTry > 3)
	{
		lastRs2OpenTry = TimeStamp;
		rs2Opened = rs2grabber.openRs2();

		if (rs2Opened)
		{
			ofLogVerbose("Rs2Projector") << "Rs2Projector.update(): A Rs2 was found ";
			rs2Res = rs2grabber.getRs2Size();
			rs2ROI = ofRectangle(0, 0, rs2Res.x, rs2Res.y);
			ofLogVerbose("Rs2Projector") << "Rs2Projector.update(): rs2ROI " << rs2ROI;

			rs2grabber.setupFramefilter(gradFieldResolution, maxOffset, rs2ROI, spatialFiltering, followBigChanges, numAveragingSlots);
			rs2WorldMatrix = rs2grabber.getWorldMatrix();
			ofLogVerbose("Rs2Projector") << "Rs2Projector.update(): rs2WorldMatrix: " << rs2WorldMatrix;
            
            
			
		}
	}

	if (displayGui)
	{
		gui->update();
		StatusGUI->update();
	}

    // Get images from rs2 grabber
    ofFloatPixels filteredframe;
    if (rs2Opened && rs2grabber.filtered.tryReceive(filteredframe))
	{
		fpsRs2.newFrame();
		fpsRs2Text->setText(ofToString(fpsRs2.getFps(), 2));

		FilteredDepthImage.setFromPixels(filteredframe.getData(), rs2Res.x, rs2Res.y);
        FilteredDepthImage.updateTexture();
        
        // Get color image from rs2 grabber
        ofPixels coloredframe;
        if (rs2grabber.colored.tryReceive(coloredframe))
		{
            rs2ColorImage.setFromPixels(coloredframe);
		
			if (TemporalFilteringType == 0)
				TemporalFrameFilter.NewFrame(rs2ColorImage.getPixels().getData(), rs2ColorImage.width, rs2ColorImage.height);
			else if (TemporalFilteringType == 1)
				TemporalFrameFilter.NewColFrame(rs2ColorImage.getPixels().getData(), rs2ColorImage.width, rs2ColorImage.height);
		}

        // Get gradient field from rs2 grabber
        rs2grabber.gradient.tryReceive(gradField);
        
        // Update grabber stored frame number
        rs2grabber.lock();
        rs2grabber.decStoredframes();
        rs2grabber.unlock();
        
        // Is the depth image stabilized
        imageStabilized = rs2grabber.isImageStabilized();
        
        // Are we calibrating ?
        if (applicationState == APPLICATION_STATE_CALIBRATING && !waitingForFlattenSand) 
		{
            updateCalibration();
        } 
		else 
		{
			fboMainWindow.begin();
            if (drawRs2View || drawRs2ColorView)
			{
				if (drawRs2ColorView)
				{
					rs2ColorImage.updateTexture();
					rs2ColorImage.draw(0, 0);
				}
				else
				{
					FilteredDepthImage.draw(0, 0);
				}
				ofNoFill();
				
				if (ROIcalibrated)
				{
					ofSetColor(0, 0, 255);
					ofDrawRectangle(rs2ROI);
				}

				ofSetColor(255, 0, 0);
				ofDrawRectangle(1, 1, rs2Res.x-1, rs2Res.y-1);
		
				if (calibrationState == CALIBRATION_STATE_ROI_MANUAL_DETERMINATION && ROICalibState == ROI_CALIBRATION_STATE_INIT)
				{
					int xmin = std::min((int)ROIStartPoint.x, (int)ROICurrentPoint.x);
					int xmax = std::max((int)ROIStartPoint.x, (int)ROICurrentPoint.x);
					int ymin = std::min((int)ROIStartPoint.y, (int)ROICurrentPoint.y);
					int ymax = std::max((int)ROIStartPoint.y, (int)ROICurrentPoint.y);

					if (xmin >= 0) // Start point has been set
					{
						ofSetColor(0, 255, 0);
						ofRectangle tempRect(xmin, ymin, xmax - xmin, ymax - ymin);
						ofDrawRectangle(tempRect);
					}
				}
			} 
			else 
			{
                ofClear(0, 0, 0, 0);
            }
            fboMainWindow.end();
        }
    }

	fboProjWindow.begin();

	if (applicationState != APPLICATION_STATE_CALIBRATING)
	{
		ofClear(255, 255, 255, 0);
	}
	if (doShowROIonProjector && ROIcalibrated && rs2Opened)
	{
		ofNoFill();
		ofSetLineWidth(4);

		// Draw rectangle of ROI using the offset by the current sand level
		ofVec2f UL = rs2CoordToProjCoord(rs2ROI.getMinX(), rs2ROI.getMinY());
		ofVec2f LR = rs2CoordToProjCoord(rs2ROI.getMaxX()-1, rs2ROI.getMaxY()-1);

		ofSetColor(255, 0, 0);
		ofRectangle tempRect(ofPoint(UL.x, UL.y), ofPoint(LR.x, LR.y));
		ofDrawRectangle(tempRect);		

		ofSetColor(0, 0, 255);
		ofRectangle tempRect2(ofPoint(UL.x - 2, UL.y - 2), ofPoint(UL.x + 2, UL.y + 2));
		ofDrawRectangle(tempRect2);
        basePlaneOffset.z = (int)basePlaneOffset.z;

		// Draw rectangle of ROI using the offset by the waterlevel
		UL = rs2CoordToProjCoord(rs2ROI.getMinX(), rs2ROI.getMinY(), basePlaneOffset.z);
		LR = rs2CoordToProjCoord(rs2ROI.getMaxX(), rs2ROI.getMaxY(), basePlaneOffset.z);

		ofSetColor(0, 255, 0);
		tempRect = ofRectangle(ofPoint(UL.x, UL.y), ofPoint(LR.x, LR.y));
		ofDrawRectangle(tempRect);

		ofSetColor(255, 0, 255);
		tempRect2 = ofRectangle(ofPoint(UL.x - 2, UL.y - 2), ofPoint(UL.x + 2, UL.y + 2));
		ofDrawRectangle(tempRect2);
	}
	else if (applicationState == APPLICATION_STATE_SETUP)
	{
		ofBackground(255); // Set to white in setup mode
	}
	fboProjWindow.end();
}

void Rs2Projector::mousePressed(int x, int y, int button)
{
	if (calibrationState == CALIBRATION_STATE_ROI_MANUAL_DETERMINATION && ROICalibState == ROI_CALIBRATION_STATE_INIT)
	{
		ROIStartPoint.x = x;
		ROIStartPoint.y = y;
		ROICurrentPoint.x = x;
		ROICurrentPoint.y = y;
	}
	else if (rs2Opened && drawRs2View)
	{
		int ind = y * rs2Res.x + x;
		if (ind >= 0 && ind < FilteredDepthImage.getFloatPixelsRef().getTotalBytes())
		{
			float z = FilteredDepthImage.getFloatPixelsRef().getData()[ind];
			std::cout << "Rs2 depth (x, y, z) = (" << x << ", " << y << ", " << z << ")" << std::endl;
		}
	}
}


void Rs2Projector::mouseReleased(int x, int y, int button)
{
	if (calibrationState == CALIBRATION_STATE_ROI_MANUAL_DETERMINATION && ROICalibState == ROI_CALIBRATION_STATE_INIT)
	{
		if (ROIStartPoint.x >= 0)
		{
			x = std::max(0, x);
			x = std::min((int)rs2Res.x - 1, x);
			y = std::max(0, y);
			y = std::min((int)rs2Res.y - 1, y);

			ROICurrentPoint.x = x;
			ROICurrentPoint.y = y;

			int xmin = std::min((int)ROIStartPoint.x, (int)ROICurrentPoint.x);
			int xmax = std::max((int)ROIStartPoint.x, (int)ROICurrentPoint.x);
			int ymin = std::min((int)ROIStartPoint.y, (int)ROICurrentPoint.y);
			int ymax = std::max((int)ROIStartPoint.y, (int)ROICurrentPoint.y);

			ofRectangle tempRect(xmin, ymin, xmax - xmin, ymax - ymin);
			rs2ROI = tempRect;
			setNewRs2ROI();
			ROICalibState = ROI_CALIBRATION_STATE_DONE;
			calibrationText = "Manual ROI defined";
			updateStatusGUI();
		}
	}
}

void Rs2Projector::mouseDragged(int x, int y, int button)
{
	if (calibrationState == CALIBRATION_STATE_ROI_MANUAL_DETERMINATION && ROICalibState == ROI_CALIBRATION_STATE_INIT)
	{
		x = std::max(0, x);
		x = std::min((int)rs2Res.x-1, x);
		y = std::max(0, y);
		y = std::min((int)rs2Res.y - 1, y);

		ROICurrentPoint.x = x;
		ROICurrentPoint.y = y;
	}
}

bool Rs2Projector::getProjectionFlipped()
{
	return (rs2ProjMatrix(0, 0) < 0);
}


void Rs2Projector::updateCalibration()
{
    if (calibrationState == CALIBRATION_STATE_FULL_AUTO_CALIBRATION)
	{
        updateFullAutoCalibration();
    } else if (calibrationState == CALIBRATION_STATE_ROI_AUTO_DETERMINATION){
        updateROIAutoCalibration();
    }
	else if (calibrationState == CALIBRATION_STATE_PROJ_RS2_AUTO_CALIBRATION){
        updateProjRs2AutoCalibration();
    }else if (calibrationState == CALIBRATION_STATE_PROJ_RS2_MANUAL_CALIBRATION) {
        updateProjRs2ManualCalibration();
    }
}

void Rs2Projector::updateFullAutoCalibration()
{
    if (fullCalibState == FULL_CALIBRATION_STATE_ROI_DETERMINATION)
	{
		updateROIFromFile();
        if (ROICalibState == ROI_CALIBRATION_STATE_DONE) 
		{
            fullCalibState = FULL_CALIBRATION_STATE_AUTOCALIB;
            autoCalibState = AUTOCALIB_STATE_INIT_FIRST_PLANE;
        }
    } 
	else if (fullCalibState == FULL_CALIBRATION_STATE_AUTOCALIB)
	{
        updateProjRs2AutoCalibration();
        if (autoCalibState == AUTOCALIB_STATE_DONE)
		{
            fullCalibState = FULL_CALIBRATION_STATE_DONE;
        }
    }
}

void Rs2Projector::updateROIAutoCalibration()
{
    updateROIFromDepthImage();
}

void Rs2Projector::updateROIFromCalibration()
{
	ofVec2f a = worldCoordTors2Coord(projCoordAndWorldZToWorldCoord(0, 0, basePlaneOffset.z));
	ofVec2f b = worldCoordTors2Coord(projCoordAndWorldZToWorldCoord(projRes.x, 0, basePlaneOffset.z));
	ofVec2f c = worldCoordTors2Coord(projCoordAndWorldZToWorldCoord(projRes.x, projRes.y, basePlaneOffset.z));
	ofVec2f d = worldCoordTors2Coord(projCoordAndWorldZToWorldCoord(0, projRes.y, basePlaneOffset.z));
	float x1 = max(a.x, d.x);
	float x2 = min(b.x, c.x);
	float y1 = max(a.y, b.y);
	float y2 = min(c.y, d.y);
	ofRectangle smallRs2ROI = ofRectangle(ofPoint(max(x1, rs2ROI.getLeft()), max(y1, rs2ROI.getTop())), ofPoint(min(x2, rs2ROI.getRight()), min(y2, rs2ROI.getBottom())));
	rs2ROI = smallRs2ROI;

	rs2ROI.standardize();
	ofLogVerbose("Rs2Projector") << "updateROIFromCalibration(): final rs2ROI : " << rs2ROI;
	setNewRs2ROI();
}

//TODO: update color image ROI acquisition to use calibration modal
void Rs2Projector::updateROIFromColorImage()
{
    fboProjWindow.begin();
    ofBackground(255);
    fboProjWindow.end();
    if (ROICalibState == ROI_CALIBRATION_STATE_INIT) { // set rs2 to max depth range
        ROICalibState = ROI_CALIBRATION_STATE_MOVE_UP;
        large = ofPolyline();
        threshold = 90;
        
    } else if (ROICalibState == ROI_CALIBRATION_STATE_MOVE_UP) {
        while (threshold < 255){
            rs2ColorImage.setROI(0, 0, rs2Res.x, rs2Res.y);
            thresholdedImage = rs2ColorImage;
            cvThreshold(thresholdedImage.getCvImage(), thresholdedImage.getCvImage(), threshold, 255, CV_THRESH_BINARY_INV);
            contourFinder.findContours(thresholdedImage, 12, rs2Res.x*rs2Res.y, 5, true);
            ofPolyline small = ofPolyline();
            for (int i = 0; i < contourFinder.nBlobs; i++) {
                ofxCvBlob blobContour = contourFinder.blobs[i];
                if (blobContour.hole) {
                    ofPolyline poly = ofPolyline(blobContour.pts);
                    if (poly.inside(rs2Res.x/2, rs2Res.y/2))
                    {
                        if (small.size() == 0 || poly.getArea() < small.getArea()) {
                            small = poly;
                        }
                    }
                }
            }
            ofLogVerbose("Rs2Projector") << "Rs2Projector.updateROIFromColorImage(): small.getArea(): " << small.getArea() ;
            ofLogVerbose("Rs2Projector") << "Rs2Projector.updateROIFromColorImage(): large.getArea(): " << large.getArea() ;
            if (large.getArea() < small.getArea())
            {
                ofLogVerbose("Rs2Projector") << "updateROIFromColorImage(): We take the largest contour line surroundings the center of the screen at all threshold level" ;
                large = small;
            }
            threshold+=1;
        }
        rs2ROI = large.getBoundingBox();
        rs2ROI.standardize();
        ofLogVerbose("Rs2Projector") << "updateROIFromColorImage(): rs2ROI : " << rs2ROI ;
        ROICalibState = ROI_CALIBRATION_STATE_DONE;
        setNewRs2ROI();
    } else if (ROICalibState == ROI_CALIBRATION_STATE_DONE){
    }
}

void Rs2Projector::updateROIFromDepthImage(){
	int counter = 0;
    if (ROICalibState == ROI_CALIBRATION_STATE_INIT) {
        calibModal->setMessage("Enlarging acquisition area & resetting buffers.");
        setMaxRs2GrabberROI();
        calibModal->setMessage("Stabilizing acquisition.");
        ROICalibState = ROI_CALIBRATION_STATE_READY_TO_MOVE_UP;
    } else if (ROICalibState == ROI_CALIBRATION_STATE_READY_TO_MOVE_UP && imageStabilized) {
        calibModal->setMessage("Scanning depth field to find sandbox walls.");
        ofLogVerbose("Rs2Projector") << "updateROIFromDepthImage(): ROI_CALIBRATION_STATE_READY_TO_MOVE_UP: got a stable depth image" ;
        ROICalibState = ROI_CALIBRATION_STATE_MOVE_UP;
        large = ofPolyline();
        ofxCvFloatImage temp;
        temp.setFromPixels(FilteredDepthImage.getFloatPixelsRef().getData(), rs2Res.x, rs2Res.y);
        temp.setNativeScale(FilteredDepthImage.getNativeScaleMin(), FilteredDepthImage.getNativeScaleMax());
        temp.convertToRange(0, 1);
        thresholdedImage.setFromPixels(temp.getFloatPixelsRef());
        threshold = 0; // We go from the higher distance to the rs2 (lower position) to the lower distance
    } else if (ROICalibState == ROI_CALIBRATION_STATE_MOVE_UP) {
	ofLogVerbose("Rs2Projector") << "updateROIFromDepthImage(): ROI_CALIBRATION_STATE_MOVE_UP";
		while (threshold < 255){
            cvThreshold(thresholdedImage.getCvImage(), thresholdedImage.getCvImage(), 255-threshold, 255, CV_THRESH_TOZERO_INV);
            thresholdedImage.updateTexture();
            contourFinder.findContours(thresholdedImage, 12, rs2Res.x*rs2Res.y, 5, true, false);
            ofPolyline small = ofPolyline();
            for (int i = 0; i < contourFinder.nBlobs; i++) {
                ofxCvBlob blobContour = contourFinder.blobs[i];
                if (blobContour.hole) {
                    ofPolyline poly = ofPolyline(blobContour.pts);
                    if (poly.inside(rs2Res.x/2, rs2Res.y/2))
                    {
                        if (small.size() == 0 || poly.getArea() < small.getArea()) {
                            small = poly;
                        }
                    }
                }
            }
            if (large.getArea() < small.getArea())
            {
                ofLogVerbose("Rs2Projector") << "updateROIFromDepthImage(): updating ROI" ;
                large = small;
            }
            threshold+=1;
        }
        if (large.getArea() == 0)
        {
			ofLogVerbose("Rs2Projector") << "Calibration failed: The sandbox walls could not be found";
            calibModal->hide();
            confirmModal->setTitle("Calibration failed");
            confirmModal->setMessage("The sandbox walls could not be found.");
            confirmModal->show();
			applicationState = APPLICATION_STATE_SETUP;
			updateStatusGUI();
        } else {
            rs2ROI = large.getBoundingBox();
            rs2ROI.standardize();
            calibModal->setMessage("Sand area successfully detected");
            ofLogVerbose("Rs2Projector") << "updateROIFromDepthImage(): final rs2ROI : " << rs2ROI ;
            setNewRs2ROI();
            if (calibrationState == CALIBRATION_STATE_ROI_AUTO_DETERMINATION)
			{
				applicationState = APPLICATION_STATE_SETUP;
                calibModal->hide();
				updateStatusGUI();
            }
        }
        ROICalibState = ROI_CALIBRATION_STATE_DONE;
    } else if (ROICalibState == ROI_CALIBRATION_STATE_DONE){
    }
}

void Rs2Projector::updateROIFromFile()
{
	string settingsFile = "settings/rs2ProjectorSettings.xml";

	ofXml xml;
	if (xml.load(settingsFile))
	{
		xml.setTo("RS2SETTINGS");
		rs2ROI = xml.getValue<ofRectangle>("rs2ROI");
		setNewRs2ROI();
		ROICalibState = ROI_CALIBRATION_STATE_DONE;
		return;
	}
	ofLogVerbose("Rs2Projector") << "updateROIFromFile(): could not read settings/rs2ProjectorSettings.xml";
	applicationState = APPLICATION_STATE_SETUP;
	updateStatusGUI();
}

void Rs2Projector::setMaxRs2GrabberROI(){
    updateRs2GrabberROI(ofRectangle(0, 0, rs2Res.x, rs2Res.y));
}

void Rs2Projector::setNewRs2ROI()
{
	CheckAndNormalizeRs2ROI();

    // Cast to integer values
    rs2ROI.x = static_cast<int>(rs2ROI.x);
    rs2ROI.y = static_cast<int>(rs2ROI.y);
    rs2ROI.width = static_cast<int>(rs2ROI.width);
    rs2ROI.height = static_cast<int>(rs2ROI.height);
    
	ofLogVerbose("Rs2Projector") << "setNewRs2ROI : " << rs2ROI;

    // Update states variables
    ROIcalibrated = true;
    saveCalibrationAndSettings();
    updateRs2GrabberROI(rs2ROI);
	updateStatusGUI();
}

void Rs2Projector::updateRs2GrabberROI(ofRectangle ROI){
    rs2grabber.performInThread([ROI](Rs2Grabber & kg) {
        kg.setRs2ROI(ROI);
    });
    imageStabilized = false; // Now we can wait for a clean new depth frame
}

std::string Rs2Projector::GetTimeAndDateString()
{
	time_t t = time(0);   // get time now
	struct tm * now = localtime(&t);
	std::stringstream ss;

	ss << now->tm_mday << '-'
		<< (now->tm_mon + 1) << '-'
		<< (now->tm_year + 1900) << '-'
		<< now->tm_hour << '-'
		<< now->tm_min << '-'
		<< now->tm_sec;

	return ss.str();
}



bool Rs2Projector::savePointPair()
{
	std::string ppK = ofToDataPath(DebugFileOutDir + "CalibrationPointPairsRs2.txt");
	std::string ppP = ofToDataPath(DebugFileOutDir + "CalibrationPointPairsRs2.txt");
	std::ofstream ppKo(ppK);
	std::ofstream ppPo(ppP);

	for (int i = 0; i < pairsRs2.size(); i++)
	{
		ppKo << pairsRs2[i].x << " " << pairsRs2[i].y << " " << pairsRs2[i].z << " " << i <<  std::endl;
	}

	for (int i = 0; i < pairsProjector.size(); i++)
	{
		ppPo << pairsProjector[i].x << " " << pairsProjector[i].y << " " << i << std::endl;
	}
	return true;
}


void Rs2Projector::updateProjRs2AutoCalibration()
{
    if (autoCalibState == AUTOCALIB_STATE_INIT_FIRST_PLANE)
	{
        rs2grabber.performInThread([](Rs2Grabber & kg) {
            kg.setMaxOffset(0);
        });
		calibrationText = "Stabilizing acquisition";
        autoCalibState = AUTOCALIB_STATE_INIT_POINT;
		updateStatusGUI();
    } 
	else if (autoCalibState == AUTOCALIB_STATE_INIT_POINT && imageStabilized)
	{
		calibrationText = "Acquiring sea level plane";
		updateStatusGUI();
		updateBasePlane(); // Find base plane
		if (!basePlaneComputed)
		{
			applicationState = APPLICATION_STATE_SETUP;
			calibrationText = "Failed to acquire sea level plane";
			updateStatusGUI();
			return;
		}
		calibrationText = "Sea level plane estimated";
		updateStatusGUI();

        autoCalibPts = new ofPoint[15];
		float cs = 4 * chessboardSize / 3; 
		float css = 3 * chessboardSize / 4;
        ofPoint sc = ofPoint(projRes.x/2,projRes.y/2);
        
        // Prepare 10 locations for the calibration chessboard
		// With a point of (0,0) the chessboard will be placed with the center in  the center of the projector
		// a point of -sc will the chessboard will be placed with the center in the upper left corner
		// Rasmus modified sequence with a center chessboard first to check if everything is working
        autoCalibPts[0] = ofPoint(0          ,0);                 // Center
        autoCalibPts[1] = ofPoint(projRes.x-cs,           cs) - sc; // upper right
        autoCalibPts[2] = ofPoint(projRes.x-cs, projRes.y-cs) - sc; // Lower right
        autoCalibPts[3] = ofPoint(          cs, projRes.y-cs) - sc; // Lower left
        autoCalibPts[4] = ofPoint(          cs,           cs)  -sc; // upper left 
        autoCalibPts[5] = ofPoint(0         ,0);                    // Center
        autoCalibPts[6] = ofPoint(projRes.x-cs,           cs) - sc; // upper right
        autoCalibPts[7] = ofPoint(projRes.x-cs, projRes.y-cs) - sc; // Lower right
        autoCalibPts[8] = ofPoint(          cs, projRes.y-cs) - sc; // Lower left
        autoCalibPts[9] = ofPoint(          cs,           cs)  -sc; // upper left
        autoCalibPts[10] = ofPoint(0         ,0);                    // Center
        autoCalibPts[11] = ofPoint(projRes.x-css,         css) - sc; // upper right
        autoCalibPts[12] = ofPoint(projRes.x-css,projRes.y-css) -sc; // Lower right
        autoCalibPts[13] = ofPoint(css          ,projRes.y-css) -sc; // Lower left
        autoCalibPts[14] = ofPoint(css,                    css) - sc; // upper left

		currentCalibPts = 0;
        upframe = false;
        trials = 0;
		TemporalFrameCounter = 0;

		ofPoint dispPt = ofPoint(projRes.x / 2, projRes.y / 2) + autoCalibPts[currentCalibPts]; //
		drawChessboard(dispPt.x, dispPt.y, chessboardSize); // We can now draw the next chess board

        autoCalibState = AUTOCALIB_STATE_NEXT_POINT;
    } 
	else if (autoCalibState == AUTOCALIB_STATE_NEXT_POINT && imageStabilized)
	{
		if (!(TemporalFrameCounter % 20))
			ofLogVerbose("Rs2Projector") << "autoCalib(): Got frame " + ofToString(TemporalFrameCounter) + " / " + ofToString(TemporalFrameFilter.getBufferSize() + 3) + " for temporal filter";

		// We want to have a buffer of images that are only focusing on one chess pattern
		if (TemporalFrameCounter++ > TemporalFrameFilter.getBufferSize() + 3)
		{
			CalibrateNextPoint();
			TemporalFrameCounter = 0;
		}
	}
	else if (autoCalibState == AUTOCALIB_STATE_COMPUTE) 
	{
        updateRs2GrabberROI(rs2ROI); // Goes back to rs2ROI and maxoffset
        rs2grabber.performInThread([this](Rs2Grabber & kg) {
            kg.setMaxOffset(this->maxOffset);
        });
        if (pairsRs2.size() == 0) {
            ofLogVerbose("Rs2Projector") << "autoCalib(): Error: No points acquired !!" ;
			calibrationText = "Calibration failed: No points acquired";
			applicationState = APPLICATION_STATE_SETUP;
			updateStatusGUI();
        } 
		else 
		{
            ofLogVerbose("Rs2Projector") << "autoCalib(): Calibrating" ;
            kpt->calibrate(pairsRs2, pairsProjector);
            rs2ProjMatrix = kpt->getProjectionMatrix();

			double ReprojectionError = ComputeReprojectionError(DumpDebugFiles);
            errorcounts = ReprojectionError;
			ofLogVerbose("Rs2Projector") << "autoCalib(): ReprojectionError " + ofToString(ReprojectionError);

			if (ReprojectionError > 50)
			{
				ofLogVerbose("Rs2Projector") << "autoCalib(): ReprojectionError too big. Something wrong with projection matrix";
				projRs2Calibrated = false;
				projRs2CalibrationUpdated = false;
				applicationState = APPLICATION_STATE_SETUP;
				calibrationText = "Calibration failed - reprojection error too big";
				updateStatusGUI();
				return;
			}
            init_FBOprojector();

			// Rasmus update - I am not sure it is good to override the manual ROI
			// updateROIFromCalibration(); // Compute the limite of the ROI according to the projected area 
            projRs2Calibrated = true; // Update states variables
            projRs2CalibrationUpdated = true;
			applicationState = APPLICATION_STATE_SETUP;
			calibrationText = "Calibration successful";
			if (kpt->saveCalibration("settings/calibration.xml"))
			{
				ofLogVerbose("Rs2Projector") << "update(): initialisation: Calibration saved ";
			}
			else {
				ofLogVerbose("Rs2Projector") << "update(): initialisation: Calibration could not be saved ";
			}
			updateStatusGUI();
        }
        autoCalibState = AUTOCALIB_STATE_DONE;
    }
	else if (!imageStabilized)
	{
		ofLogVerbose("Rs2Projector") << "updateProjRs2AutoCalibration(): image not stabilised";
	}
	else if (autoCalibState == AUTOCALIB_STATE_DONE)
	{
    }
}

// Compute the error when using the projection matrix to project calibration Rs2 points into Project space
// and comparing with calibration projector points
double Rs2Projector::ComputeReprojectionError(bool WriteFile)
{
	std::string oErrors = ofToDataPath(DebugFileOutDir + "CalibrationReprojectionErrors_" + GetTimeAndDateString() + ".txt");

	double PError = 0;

	for (int i = 0; i < pairsRs2.size(); i++)
	{
		ofVec4f wc = pairsRs2[i];
		wc.w = 1;

		ofVec4f screenPos = rs2ProjMatrix*wc;
		ofVec2f projectedPoint(screenPos.x / screenPos.z, screenPos.y / screenPos.z);
		ofVec2f projP = pairsProjector[i];

		double D = sqrt((projectedPoint.x - projP.x) * (projectedPoint.x - projP.x) + (projectedPoint.y - projP.y) * (projectedPoint.y - projP.y));

		PError += D;
	}
	PError /= (double)pairsRs2.size();

	if (WriteFile)
	{
		std::ofstream fost2(oErrors.c_str());

		for (int i = 0; i < pairsRs2.size(); i++)
		{
			ofVec4f wc = pairsRs2[i];
			wc.w = 1;

			ofVec4f screenPos = rs2ProjMatrix*wc;
			ofVec2f projectedPoint(screenPos.x / screenPos.z, screenPos.y / screenPos.z);
			ofVec2f projP = pairsProjector[i];

			double D = sqrt((projectedPoint.x - projP.x) * (projectedPoint.x - projP.x) + (projectedPoint.y - projP.y) * (projectedPoint.y - projP.y));

			fost2 << wc.x << ", " << wc.y << ", " << wc.z << ", "
				<< projP.x << ", " << projP.y << ", " << projectedPoint.x << ", " << projectedPoint.y << ", " << D << std::endl;
		}
	}

	return PError;
}

void Rs2Projector::CalibrateNextPoint()
{
	if (currentCalibPts < 5 || (upframe && currentCalibPts < 15))
	{
		if (!upframe)
		{
			calibrationText = "Calibration (low) # " + std::to_string(currentCalibPts + 1) + "/5";
			updateStatusGUI();
		}
		else
		{
			calibrationText = "Calibration (high) #  " + std::to_string(currentCalibPts - 4) + "/10";
			updateStatusGUI();
		}

		// Current RGB frame - probably with rolling shutter problems
		cvRgbImage = ofxCv::toCv(rs2ColorImage.getPixels());

		ofxCvGrayscaleImage tempImage;
		if (TemporalFilteringType == 0)
			tempImage.setFromPixels(TemporalFrameFilter.getMedianFilteredImage(), rs2ColorImage.width, rs2ColorImage.height);
		if (TemporalFilteringType == 1)
			tempImage.setFromPixels(TemporalFrameFilter.getAverageFilteredColImage(), rs2ColorImage.width, rs2ColorImage.height);
		
		ProcessChessBoardInput(tempImage);

		if (DumpDebugFiles)
		{
			std::string tname = DebugFileOutDir + "ChessboardImage_" + GetTimeAndDateString() + "_" + ofToString(currentCalibPts) + "_try_" + ofToString(trials) + ".png";
			ofSaveImage(tempImage.getPixels(), tname);
		}

		cvGrayImage = ofxCv::toCv(tempImage.getPixels());

		cv::Rect tempROI((int)rs2ROI.x, (int)rs2ROI.y,(int)rs2ROI.width, (int)rs2ROI.height);
		cv::Mat cvGrayROI = cvGrayImage(tempROI);

		cv::Size patternSize = cv::Size(chessboardX - 1, chessboardY - 1);
		int chessFlags = 0;

		bool foundChessboard = findChessboardCorners(cvGrayROI, patternSize, cvPoints, chessFlags);

		if (!foundChessboard)
		{
			int chessFlags = cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FAST_CHECK;
			foundChessboard = findChessboardCorners(cvGrayROI, patternSize, cvPoints, chessFlags);
		}

		// Changed logic so the "cleared" flag is not used - we do a long frame average instead
		if (foundChessboard)
		{
			for (int i = 0; i < cvPoints.size(); i++)
			{
				cvPoints[i].x += tempROI.x;
				cvPoints[i].y += tempROI.y;
			}

			cornerSubPix(cvGrayImage, cvPoints, cv::Size(2, 2), cv::Size(-1, -1),   // Rasmus: changed search size to 2 from 11 - since this caused false findings
				cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			drawChessboardCorners(cvRgbImage, patternSize, cv::Mat(cvPoints), foundChessboard);

			if (DumpDebugFiles)
			{
				std::string tname = DebugFileOutDir + "FoundChessboard_" + GetTimeAndDateString() + "_" + ofToString(currentCalibPts) + "_try_" + ofToString(trials) + ".png";
				ofSaveImage(rs2ColorImage.getPixels(), tname);
			}

			rs2ColorImage.updateTexture();
			fboMainWindow.begin();
			rs2ColorImage.draw(0, 0);
			fboMainWindow.end();

			ofLogVerbose("Rs2Projector") << "autoCalib(): Chessboard found for point :" << currentCalibPts;
			bool okchess = addPointPair();

			if (okchess)
			{
				trials = 0;
				currentCalibPts++;
				ofPoint dispPt = ofPoint(projRes.x / 2, projRes.y / 2) + autoCalibPts[currentCalibPts]; // Compute next chessboard position
				drawChessboard(dispPt.x, dispPt.y, chessboardSize); // We can now draw the next chess board
			}
			else
			{
				// We cannot get all depth points for the chessboard
				trials++;
				ofLogVerbose("Rs2Projector") << "autoCalib(): Depth points of chessboard not allfound on trial : " << trials;
				if (trials > 3)
				{
					// Move the chessboard closer to the center of the screen
					ofLogVerbose("Rs2Projector") << "autoCalib(): Chessboard could not be found moving chessboard closer to center ";
					autoCalibPts[currentCalibPts] = 3 * autoCalibPts[currentCalibPts] / 4;
					ofPoint dispPt = ofPoint(projRes.x / 2, projRes.y / 2) + autoCalibPts[currentCalibPts]; // Compute next chessboard position
					drawChessboard(dispPt.x, dispPt.y, chessboardSize); // We can now draw the next chess board
					trials = 0;
				}
			}
		}
		else
		{
			// We cannot find the chessboard
			trials++;
			ofLogVerbose("Rs2Projector") << "autoCalib(): Chessboard not found on trial : " << trials;
			if (trials > 3) 
			{
				// Move the chessboard closer to the center of the screen
				ofLogVerbose("Rs2Projector") << "autoCalib(): Chessboard could not be found moving chessboard closer to center ";
				autoCalibPts[currentCalibPts] = 3 * autoCalibPts[currentCalibPts] / 4;

				ofPoint dispPt = ofPoint(projRes.x / 2, projRes.y / 2) + autoCalibPts[currentCalibPts]; // Compute next chessboard position
				drawChessboard(dispPt.x, dispPt.y, chessboardSize); // We can now draw the next chess board
				trials = 0;
			}
		}
	}
	else
	{
		if (upframe)
		{ // We are done
			calibrationText = "Updating acquisition ceiling";
			updateMaxOffset(); // Find max offset
			autoCalibState = AUTOCALIB_STATE_COMPUTE;
			updateStatusGUI();
		}
		else
		{ // We ask for higher points
			calibModal->hide();
			confirmModal->show();
			confirmModal->setMessage("Please cover the sandbox with a board and press ok.");
		}
	}
}

//TODO: Add manual Prj Rs2 calibration
void Rs2Projector::updateProjRs2ManualCalibration(){
    // Draw a Chessboard
    drawChessboard(ofGetMouseX(), ofGetMouseY(), chessboardSize);
    // Try to find the chess board on the rs2 color image
    cvRgbImage = ofxCv::toCv(rs2ColorImage.getPixels());
    cv::Size patternSize = cv::Size(chessboardX-1, chessboardY-1);
    int chessFlags = cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_FAST_CHECK;
    bool foundChessboard = findChessboardCorners(cvRgbImage, patternSize, cvPoints, chessFlags);
    if(foundChessboard) {
        cv::Mat gray;
        cvtColor(cvRgbImage, gray, CV_RGB2GRAY);
        cornerSubPix(gray, cvPoints, cv::Size(11, 11), cv::Size(-1, -1),
                     cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
        drawChessboardCorners(cvRgbImage, patternSize, cv::Mat(cvPoints), foundChessboard);
    }
}

void Rs2Projector::updateBasePlane()
{
	basePlaneComputed = false;
	updateStatusGUI();

    ofRectangle smallROI = rs2ROI;
    smallROI.scaleFromCenter(0.75); // Reduce ROI to avoid problems with borders
    ofLogVerbose("Rs2Projector") << "updateBasePlane(): smallROI: " << smallROI ;
    int sw = static_cast<int>(smallROI.width);
    int sh = static_cast<int>(smallROI.height);
    int sl = static_cast<int>(smallROI.getLeft());
    int st = static_cast<int>(smallROI.getTop());
    ofLogVerbose("Rs2Projector") << "updateBasePlane(): sw: " << sw << " sh : " << sh << " sl : " << sl << " st : " << st << " sw*sh : " << sw*sh ;
    if (sw*sh == 0) {
        ofLogVerbose("Rs2Projector") << "updateBasePlane(): smallROI is null, cannot compute base plane normal" ;
        return;
    }
    ofVec4f pt;
    ofVec3f* points;
    points = new ofVec3f[sw*sh];
    ofLogVerbose("Rs2Projector") << "updateBasePlane(): Computing points in smallROI : " << sw*sh ;
    for (int x = 0; x<sw; x++){
        for (int y = 0; y<sh; y ++){
            points[x+y*sw] = rs2CoordToWorldCoord(x+sl, y+st);
        }
    }
    ofLogVerbose("Rs2Projector") << "updateBasePlane(): Computing plane from points" ;
    basePlaneEq = plane_from_points(points, sw*sh);
	if (basePlaneEq.x == 0 && basePlaneEq.y == 0 && basePlaneEq.z == 0)
	{
		ofLogVerbose("Rs2Projector") << "updateBasePlane(): plane_from_points could not compute basePlane";
		return;
	}

    basePlaneNormal = ofVec3f(basePlaneEq);
    basePlaneOffset = ofVec3f(0,0,-basePlaneEq.w);
    basePlaneNormalBack = basePlaneNormal;
    basePlaneOffsetBack = basePlaneOffset;
    basePlaneUpdated = true;
	basePlaneComputed = true;
	updateStatusGUI();
}

void Rs2Projector::updateMaxOffset(){
    ofRectangle smallROI = rs2ROI;
    smallROI.scaleFromCenter(0.75); // Reduce ROI to avoid problems with borders
    ofLogVerbose("Rs2Projector") << "updateMaxOffset(): smallROI: " << smallROI ;
    int sw = static_cast<int>(smallROI.width);
    int sh = static_cast<int>(smallROI.height);
    int sl = static_cast<int>(smallROI.getLeft());
    int st = static_cast<int>(smallROI.getTop());
    ofLogVerbose("Rs2Projector") << "updateMaxOffset(): sw: " << sw << " sh : " << sh << " sl : " << sl << " st : " << st << " sw*sh : " << sw*sh ;
    if (sw*sh == 0) {
        ofLogVerbose("Rs2Projector") << "updateMaxOffset(): smallROI is null, cannot compute base plane normal" ;
        return;
    }
    ofVec4f pt;
    ofVec3f* points;
    points = new ofVec3f[sw*sh];
    ofLogVerbose("Rs2Projector") << "updateMaxOffset(): Computing points in smallROI : " << sw*sh ;
    for (int x = 0; x<sw; x++){
        for (int y = 0; y<sh; y ++){
            points[x+y*sw] = rs2CoordToWorldCoord(x+sl, y+st);//vertexCcvertexCc;
        }
    }
    ofLogVerbose("Rs2Projector") << "updateMaxOffset(): Computing plane from points" ;
    ofVec4f eqoff = plane_from_points(points, sw*sh);
    maxOffset = -eqoff.w-maxOffsetSafeRange;
    maxOffsetBack = maxOffset;
    // Update max Offset
    ofLogVerbose("Rs2Projector") << "updateMaxOffset(): maxOffset" << maxOffset ;
    rs2grabber.performInThread([this](Rs2Grabber & kg) {
        kg.setMaxOffset(this->maxOffset);
    });
}

bool Rs2Projector::addPointPair() {
    bool okchess = true;
    string resultMessage;
    ofLogVerbose("Rs2Projector") << "addPointPair(): Adding point pair in rs2 world coordinates" ;
    int nDepthPoints = 0;
    for (int i=0; i<cvPoints.size(); i++) {
        ofVec3f worldPoint = rs2CoordToWorldCoord(cvPoints[i].x, cvPoints[i].y);
        if (worldPoint.z > 0)   nDepthPoints++;
    }
    if (nDepthPoints == (chessboardX-1)*(chessboardY-1)) {
        for (int i=0; i<cvPoints.size(); i++) {
            ofVec3f worldPoint = rs2CoordToWorldCoord(cvPoints[i].x, cvPoints[i].y);
            pairsRs2.push_back(worldPoint);
            pairsProjector.push_back(currentProjectorPoints[i]);
        }
        resultMessage = "addPointPair(): Added " + ofToString((chessboardX-1)*(chessboardY-1)) + " points pairs.";
		if (DumpDebugFiles)
		{
			savePointPair();
		}
    } else {
        resultMessage = "addPointPair(): Points not added because not all chessboard\npoints' depth known. Try re-positionining.";
        okchess = false;
    }
    ofLogVerbose("Rs2Projector") << resultMessage ;
    return okchess;
}

void Rs2Projector::askToFlattenSand(){
    fboProjWindow.begin();
    ofBackground(255);
    fboProjWindow.end();
    confirmModal->setMessage("Please flatten the sand surface.");
    confirmModal->show();
    waitingForFlattenSand = true;
}

void Rs2Projector::drawProjectorWindow(){
    fboProjWindow.draw(0,0);
}

void Rs2Projector::drawMainWindow(float x, float y, float width, float height){

	bool forceScale = false;
	if (forceScale)
	{
		fboMainWindow.draw(x, y, width, height);
	}
	else
	{
		fboMainWindow.draw(x, y);
	}

	if (displayGui)
	{
        StatusGUI->draw();
		gui->draw();
	}
}

void Rs2Projector::init_FBOprojector(){
    fboProjWindow.allocate(projRes.x, projRes.y, GL_RGBA);
    fboProjWindow.begin();
    ofClear(255, 255, 255, 0);
    ofBackground(255); // Set to white in setup mode
    fboProjWindow.end();
}

void Rs2Projector::drawChessboard(int x, int y, int chessboardSize) {
    init_FBOprojector();
    fboProjWindow.begin();
    ofFill();
    // Draw the calibration chess board on the projector window
    float w = chessboardSize / chessboardX;
    float h = chessboardSize / chessboardY;
    
    float xf = x-chessboardSize/2; // x and y are chess board center size
    float yf = y-chessboardSize/2;
    
    currentProjectorPoints.clear();
    
	ofClear(255, 255, 255, 0);
	ofBackground(255); 
	ofSetColor(0);
    ofTranslate(xf, yf);
    for (int j=0; j<chessboardY; j++) {
        for (int i=0; i<chessboardX; i++) {
            int x0 = ofMap(i, 0, chessboardX, 0, chessboardSize);
            int y0 = ofMap(j, 0, chessboardY, 0, chessboardSize);
            if (j>0 && i>0) {
                currentProjectorPoints.push_back(ofVec2f(xf+x0, yf+y0));
            }
            if ((i+j)%2==0) ofDrawRectangle(x0, y0, w, h);
        }
    }
    ofSetColor(255);
    fboProjWindow.end();
}

void Rs2Projector::drawGradField()
{
    ofClear(255, 0);
    for(int rowPos=0; rowPos< gradFieldrows ; rowPos++)
    {
        for(int colPos=0; colPos< gradFieldcols ; colPos++)
        {
            float x = colPos*gradFieldResolution + gradFieldResolution/2;
            float y = rowPos*gradFieldResolution  + gradFieldResolution/2;
            ofVec2f projectedPoint = rs2CoordToProjCoord(x, y);
            int ind = colPos + rowPos * gradFieldcols;
            ofVec2f v2 = gradField[ind];
            v2 *= arrowLength;

            ofSetColor(255,0,0,255);
            if (ind == fishInd)
                ofSetColor(0,255,0,255);
            
            drawArrow(projectedPoint, v2);
        }
    }
}

void Rs2Projector::drawArrow(ofVec2f projectedPoint, ofVec2f v1)
{
    float angle = ofRadToDeg(atan2(v1.y,v1.x));
    float length = v1.length();
    ofFill();
    ofPushMatrix();
    ofTranslate(projectedPoint);
    ofRotate(angle);
    ofSetColor(255,0,0,255);
    ofDrawLine(0, 0, length, 0);
    ofDrawLine(length, 0, length-7, 5);
    ofDrawLine(length, 0, length-7, -5);
    ofPopMatrix();
}

void Rs2Projector::updateNativeScale(float scaleMin, float scaleMax){
    FilteredDepthImage.setNativeScale(scaleMin, scaleMax);
}

ofVec2f Rs2Projector::rs2CoordToProjCoord(float x, float y) // x, y in rs2 pixel coord
{
    return worldCoordToProjCoord(rs2CoordToWorldCoord(x, y));
}

ofVec2f Rs2Projector::rs2CoordToProjCoord(float x, float y, float z)
{
	ofVec4f kc = ofVec2f(x, y);
	kc.z = z;
	kc.w = 1;
	ofVec4f wc = rs2WorldMatrix*kc*kc.z;

	return worldCoordToProjCoord(wc);
}

ofVec2f Rs2Projector::worldCoordToProjCoord(ofVec3f vin)
{
    ofVec4f wc = vin;
    wc.w = 1;
    ofVec4f screenPos = rs2ProjMatrix*wc;
    ofVec2f projectedPoint(screenPos.x/screenPos.z, screenPos.y/screenPos.z);
    return projectedPoint;
}

ofVec3f Rs2Projector::projCoordAndWorldZToWorldCoord(float projX, float projY, float worldZ)
{
	float a = rs2ProjMatrix(0, 0) - rs2ProjMatrix(2, 0)*projX;
	float b = rs2ProjMatrix(0, 1) - rs2ProjMatrix(2, 1)*projX;
	float c = (rs2ProjMatrix(2, 2)*worldZ + 1)*projX - (rs2ProjMatrix(0, 2)*worldZ + rs2ProjMatrix(0, 3));
	float d = rs2ProjMatrix(1, 0) - rs2ProjMatrix(2, 0)*projY;
	float e = rs2ProjMatrix(1, 1) - rs2ProjMatrix(2, 1)*projY;
	float f = (rs2ProjMatrix(2, 2)*worldZ + 1)*projY - (rs2ProjMatrix(1, 2)*worldZ + rs2ProjMatrix(1, 3));
	
	float det = a*e - b*d;
	if (det == 0)
		return ofVec3f(0);
	float y = (a*f - d*c) / det;
	float x = (c*e - b*f) / det;
	return ofVec3f(x, y, worldZ);
}

ofVec3f Rs2Projector::rs2CoordToWorldCoord(float x, float y) // x, y in rs2 pixel coord
{
	// Simple crash avoidence
	if (y < 0)
		y = 0;
	if (y >= rs2Res.y)
		y = rs2Res.y - 1;
	if (x < 0)
		x = 0;
	if (x >= rs2Res.x)
		x = rs2Res.x - 1;

    ofVec4f kc = ofVec2f(x, y);
    int ind = static_cast<int>(y) * rs2Res.x + static_cast<int>(x);
    kc.z = FilteredDepthImage.getFloatPixelsRef().getData()[ind];

    kc.w = 1;
    ofVec4f wc = rs2WorldMatrix*kc*kc.z;
    return ofVec3f(wc);
}

ofVec2f Rs2Projector::worldCoordTors2Coord(ofVec3f wc)
{
	float x = (wc.x / wc.z - rs2WorldMatrix(0, 3)) / rs2WorldMatrix(0, 0);
	float y = (wc.y / wc.z - rs2WorldMatrix(1, 3)) / rs2WorldMatrix(1, 1);
	return ofVec2f(x, y);
}

ofVec3f Rs2Projector::Rawrs2CoordToWorldCoord(float x, float y) // x, y in rs2 pixel coord
{
    ofVec4f kc = ofVec3f(x, y, rs2grabber.getRawDepthAt(static_cast<int>(x), static_cast<int>(y)));
    kc.w = 1;
    ofVec4f wc = rs2WorldMatrix*kc*kc.z;
    return ofVec3f(wc);
}

float Rs2Projector::elevationAtrs2Coord(float x, float y) // x, y in rs2 pixel coordinate
{
    ofVec4f wc = rs2CoordToWorldCoord(x, y);
    wc.w = 1;
    float elevation = -basePlaneEq.dot(wc);
    return elevation;
}

float Rs2Projector::elevationTors2Depth(float elevation, float x, float y) // x, y in rs2 pixel coordinate
{
    ofVec4f wc = rs2CoordToWorldCoord(x, y);
    wc.z = 0;
    wc.w = 1;
    float rs2Depth = -(basePlaneEq.dot(wc)+elevation)/basePlaneEq.z;
    return rs2Depth;
}

ofVec2f Rs2Projector::gradientAtrs2Coord(float x, float y){
    int ind = static_cast<int>(floor(x/gradFieldResolution)) + gradFieldcols*static_cast<int>(floor(y/gradFieldResolution));
    fishInd = ind;
    return gradField[ind];
}

void Rs2Projector::setupGui(){
    // instantiate and position the gui //
    gui = new ofxDatGui( ofxDatGuiAnchor::TOP_RIGHT );
	gui->addButton("RUN!")->setName("Start Application");
	gui->addBreak();
    gui->addFRM();
	fpsRs2Text = gui->addTextInput("Rs2 FPS", "0");
    gui->addBreak();
    
    auto advancedFolder = gui->addFolder("Advanced", ofColor::purple);
    advancedFolder->addToggle("Display rs2 depth view", drawRs2View)->setName("Draw rs2 depth view");
	advancedFolder->addToggle("Display rs2 color view", drawRs2ColorView)->setName("Draw rs2 color view");
	advancedFolder->addToggle("Dump Debug", DumpDebugFiles);
	advancedFolder->addSlider("Ceiling", -300, 300, 0);
    advancedFolder->addToggle("Spatial filtering", spatialFiltering);
	advancedFolder->addToggle("Inpaint outliers", doInpainting);
	advancedFolder->addToggle("Full Frame Filtering", doFullFrameFiltering);
	advancedFolder->addToggle("Quick reaction", followBigChanges);
    advancedFolder->addSlider("Averaging", 1, 40, numAveragingSlots)->setPrecision(0);
	advancedFolder->addSlider("Tilt X", -30, 30, 0);
	advancedFolder->addSlider("Tilt Y", -30, 30, 0);
	advancedFolder->addSlider("Vertical offset", -100, 100, 0);
	advancedFolder->addButton("Reset sea level");
	advancedFolder->addBreak();
	
	auto calibrationFolder = gui->addFolder("Calibration", ofColor::darkCyan);
	calibrationFolder->addButton("Manually define sand region");
	calibrationFolder->addButton("Automatically calibrate rs2 & projector");
	calibrationFolder->addButton("Auto Adjust ROI");
	calibrationFolder->addToggle("Show ROI on sand", doShowROIonProjector);
    
    gui->addHeader(":: Settings ::", false);
    
    // once the gui has been assembled, register callbacks to listen for component specific events //
    gui->onButtonEvent(this, &Rs2Projector::onButtonEvent);
    gui->onToggleEvent(this, &Rs2Projector::onToggleEvent);
    gui->onSliderEvent(this, &Rs2Projector::onSliderEvent);

	// disactivate autodraw
	gui->setAutoDraw(false);

    StatusGUI = new ofxDatGui( ofxDatGuiAnchor::TOP_LEFT );
    
	StatusGUI->addLabel("Application Status");
	StatusGUI->addLabel("Rs2 Status");
    StatusGUI->addLabel("Ready Calibration");
	StatusGUI->addLabel("ROI Status");
	StatusGUI->addLabel("Baseplane Status");
	StatusGUI->addLabel("Calibration Status");
	StatusGUI->addLabel("Calibration Step");
    StatusGUI->addLabel("Calibration Error Count");
	StatusGUI->addLabel("Projector Status");
	StatusGUI->addHeader(":: Status ::", false);
    StatusGUI->addBreak();
    StatusGUI->setAutoDraw(false);
    updateStatusGUI();
}


void Rs2Projector::startApplication()
{
	if (applicationState == APPLICATION_STATE_RUNNING)
	{
		applicationState = APPLICATION_STATE_SETUP;
		updateStatusGUI();
		return;
	}
	if (applicationState == APPLICATION_STATE_CALIBRATING)
	{
		ofLogVerbose("Rs2Projector") << "Rs2Projector.startApplication(): we are calibrating ";
		return;
	}
	if (!rs2Opened)
	{
		ofLogVerbose("Rs2Projector") << "Rs2Projector.startApplication(): Rs2 is not running ";
		return;
	}

	if (!projRs2Calibrated)
	{
		ofLogVerbose("Rs2Projector") << "Rs2Projector.startApplication(): Rs2 projector not calibrated - trying to load calibration.xml";
		//Try to load calibration file if possible
		if (kpt->loadCalibration("settings/calibration.xml"))
		{
			ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): Calibration loaded ";
			rs2ProjMatrix = kpt->getProjectionMatrix();
			ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): rs2ProjMatrix: " << rs2ProjMatrix;
			projRs2Calibrated = true;
			projRs2CalibrationUpdated = true;
			updateStatusGUI();
		}
		else
		{
			ofLogVerbose("Rs2Projector") << "Rs2Projector.startApplication(): Calibration could not be loaded";
			return;
		}
	}

	if (!ROIcalibrated)
	{
		ofLogVerbose("Rs2Projector") << "Rs2Projector.startApplication(): Rs2 ROI not calibrated - trying to load rs2ProjectorSettings.xml";
		//Try to load settings file if possible
		if (loadSettings())
		{
			ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): Settings loaded ";
			setNewRs2ROI();
			ROIcalibrated = true;
			basePlaneComputed = true;
			setFullFrameFiltering(doFullFrameFiltering);
			setInPainting(doInpainting);
			setFollowBigChanges(followBigChanges);
			setSpatialFiltering(spatialFiltering);

			int nAvg = numAveragingSlots;
			rs2grabber.performInThread([nAvg](Rs2Grabber & kg) {
				kg.setAveragingSlotsNumber(nAvg); });

			updateStatusGUI();
		}
		else 
		{
			ofLogVerbose("Rs2Projector") << "Rs2Projector.setup(): Settings could not be loaded ";
			return;
		}
	}

	ResetSeaLevel();

	// If all is well we are running
	applicationState = APPLICATION_STATE_RUNNING;
	fullCalibState = FULL_CALIBRATION_STATE_DONE;
	ROICalibState = ROI_CALIBRATION_STATE_DONE;
	autoCalibState = AUTOCALIB_STATE_DONE;
	drawRs2ColorView = false;
	drawRs2View = false;
	gui->getToggle("Draw rs2 color view")->setChecked(drawRs2ColorView);
	gui->getToggle("Draw rs2 depth view")->setChecked(drawRs2View);
	updateStatusGUI();
}

void Rs2Projector::startFullCalibration()
{
	if (!rs2Opened)
	{
		ofLogVerbose("Rs2Projector") << "startFullCalibration(): Rs2 not running";
		return;
	}
	if (applicationState == APPLICATION_STATE_CALIBRATING)
	{
		ofLogVerbose("Rs2Projector") << "startFullCalibration(): we are already calibrating";
		return;
	}

	applicationState = APPLICATION_STATE_CALIBRATING;
    calibrationState = CALIBRATION_STATE_FULL_AUTO_CALIBRATION;
    fullCalibState = FULL_CALIBRATION_STATE_ROI_DETERMINATION;
	ROICalibState = ROI_CALIBRATION_STATE_INIT;
	confirmModal->setTitle("Full calibration");
    calibModal->setTitle("Full calibration");
    askToFlattenSand();
    ofLogVerbose("Rs2Projector") << "startFullCalibration(): Starting full calibration" ;
	updateStatusGUI();
}

void Rs2Projector::startAutomaticROIDetection(){
	applicationState = APPLICATION_STATE_CALIBRATING;
    calibrationState = CALIBRATION_STATE_ROI_AUTO_DETERMINATION;
    ROICalibState = ROI_CALIBRATION_STATE_INIT;
    ofLogVerbose("Rs2Projector") << "onButtonEvent(): Finding ROI" ;
    confirmModal->setTitle("Detect sand region");
    calibModal->setTitle("Detect sand region");
    askToFlattenSand();
    ofLogVerbose("Rs2Projector") << "startAutomaticROIDetection(): starting ROI detection" ;
	updateStatusGUI();
}

void Rs2Projector::startAutomaticrs2ProjectorCalibration(){
	if (!rs2Opened)
	{
		ofLogVerbose("Rs2Projector") << "startAutomaticRs2ProjectorCalibration(): Rs2 not running";
		return;
	}
	if (applicationState == APPLICATION_STATE_CALIBRATING)
	{
		applicationState = APPLICATION_STATE_SETUP;
		calibrationText = "Terminated before completion";
		updateStatusGUI();
		return;
	}
	if (!ROIcalibrated)
	{
		ofLogVerbose("Rs2Projector") << "startAutomaticRs2ProjectorCalibration(): ROI not defined";
		return;
	}

	calibrationText = "Starting projector/rs2 calibration";

	applicationState = APPLICATION_STATE_CALIBRATING;
    calibrationState = CALIBRATION_STATE_PROJ_RS2_AUTO_CALIBRATION;
    autoCalibState = AUTOCALIB_STATE_INIT_POINT;
    confirmModal->setTitle("Calibrate projector");
    calibModal->setTitle("Calibrate projector");
    askToFlattenSand();
    ofLogVerbose("Rs2Projector") << "startAutomaticRs2ProjectorCalibration(): Starting autocalib" ;
	updateStatusGUI();
}

void Rs2Projector::setSpatialFiltering(bool sspatialFiltering){
    spatialFiltering = sspatialFiltering;
    rs2grabber.performInThread([sspatialFiltering](Rs2Grabber & kg) {
        kg.setSpatialFiltering(sspatialFiltering);
    });
	updateStatusGUI();
}

void Rs2Projector::setInPainting(bool inp) {
	doInpainting = inp;
	rs2grabber.performInThread([inp](Rs2Grabber & kg) {
		kg.setInPainting(inp);
	});
	updateStatusGUI();
}


void Rs2Projector::setFullFrameFiltering(bool ff)
{
	doFullFrameFiltering = ff;
	ofRectangle ROI = rs2ROI;
	rs2grabber.performInThread([ff, ROI](Rs2Grabber & kg) {
		kg.setFullFrameFiltering(ff, ROI);
	});
	updateStatusGUI();
}

void Rs2Projector::setFollowBigChanges(bool sfollowBigChanges){
    followBigChanges = sfollowBigChanges;
    rs2grabber.performInThread([sfollowBigChanges](Rs2Grabber & kg) {
        kg.setFollowBigChange(sfollowBigChanges);
    });
	updateStatusGUI();
}

void Rs2Projector::onButtonEvent(ofxDatGuiButtonEvent e){
    if (e.target->is("Full Calibration")) {
        startFullCalibration();
    } 
	else if (e.target->is("Start Application"))
	{
		startApplication();
	}
	else if (e.target->is("Update ROI from calibration")) {
		updateROIFromCalibration();
	} else if (e.target->is("Automatically detect sand region")) {
        startAutomaticROIDetection();
    } else if (e.target->is("Manually define sand region")){
		StartManualROIDefinition();
	}
	else if (e.target->is("Automatically calibrate rs2 & projector")) {
        startAutomaticrs2ProjectorCalibration();
    } else if (e.target->is("Manually calibrate rs2 & projector")) {
        // Not implemented yet
    } else if (e.target->is("Reset sea level")){
		ResetSeaLevel();

    }
	else if (e.target->is("Auto Adjust ROI"))
	{
		updateROIFromCalibration();
	}
}

void Rs2Projector::StartManualROIDefinition()
{
	calibrationState = CALIBRATION_STATE_ROI_MANUAL_DETERMINATION;
	ROICalibState = ROI_CALIBRATION_STATE_INIT;
	ROIStartPoint.x = -1;
	ROIStartPoint.y = -1;
	calibrationText = "Manually defining sand region";
	updateStatusGUI();
}

void Rs2Projector::ResetSeaLevel()
{
	gui->getSlider("Tilt X")->setValue(0);
	gui->getSlider("Tilt Y")->setValue(0);
	gui->getSlider("Vertical offset")->setValue(0);
	basePlaneNormal = basePlaneNormalBack;
	basePlaneOffset = basePlaneOffsetBack;
	basePlaneEq = getPlaneEquation(basePlaneOffset, basePlaneNormal);
	basePlaneUpdated = true;
}

void Rs2Projector::showROIonProjector(bool show)
{
	doShowROIonProjector = show;
	fboProjWindow.begin();
	ofClear(255, 255, 255, 0);
	fboProjWindow.end();
}

bool Rs2Projector::getDumpDebugFiles()
{
	return DumpDebugFiles;
}

void Rs2Projector::onToggleEvent(ofxDatGuiToggleEvent e){
    if (e.target->is("Spatial filtering")) {
		setSpatialFiltering(e.checked);
	}
	else if (e.target->is("Quick reaction")) {
		setFollowBigChanges(e.checked);
	}
	else if (e.target->is("Inpaint outliers")) {
		setInPainting(e.checked);
    } 
	else if (e.target->is("Full Frame Filtering")) {
		setFullFrameFiltering(e.checked);
	}
	else if (e.target->is("Draw rs2 depth view")){
        drawRs2View = e.checked;
		if (drawRs2View)
		{
			drawRs2ColorView = false;
			gui->getToggle("Draw rs2 color view")->setChecked(drawRs2ColorView);
		}
    }
	else if (e.target->is("Draw rs2 color view")) {
		drawRs2ColorView = e.checked;
		if (drawRs2ColorView)
		{
			drawRs2View = false;
			gui->getToggle("Draw rs2 depth view")->setChecked(drawRs2View);
		}
	}
	else if (e.target->is("Dump Debug"))
	{
		DumpDebugFiles = e.checked;
	}
	else if (e.target->is("Show ROI on sand"))
	{
		showROIonProjector(e.checked);
	}
}

void Rs2Projector::onSliderEvent(ofxDatGuiSliderEvent e){
    if (e.target->is("Tilt X") || e.target->is("Tilt Y")) {
        basePlaneNormal = basePlaneNormalBack.getRotated(gui->getSlider("Tilt X")->getValue(), ofVec3f(1,0,0));
        basePlaneNormal.rotate(gui->getSlider("Tilt Y")->getValue(), ofVec3f(0,1,0));
        basePlaneEq = getPlaneEquation(basePlaneOffset,basePlaneNormal);
        basePlaneUpdated = true;
    } else if (e.target->is("Vertical offset")) {
        basePlaneOffset.z = basePlaneOffsetBack.z + e.value;
        basePlaneEq = getPlaneEquation(basePlaneOffset,basePlaneNormal);
        basePlaneUpdated = true;
    } else if (e.target->is("Ceiling")){
        maxOffset = maxOffsetBack-e.value;
        ofLogVerbose("Rs2Projector") << "onSliderEvent(): maxOffset" << maxOffset ;
        rs2grabber.performInThread([this](Rs2Grabber & kg) {
            kg.setMaxOffset(this->maxOffset);
        });
    } else if(e.target->is("Averaging")){
        numAveragingSlots = e.value;
        rs2grabber.performInThread([e](Rs2Grabber & kg) {
            kg.setAveragingSlotsNumber(e.value);
        });
    }
}

void Rs2Projector::onConfirmModalEvent(ofxModalEvent e)
{
    if (e.type == ofxModalEvent::SHOWN)
	{
        ofLogVerbose("Rs2Projector") << "Confirm modal window is open" ;
    }  
	else if (e.type == ofxModalEvent::HIDDEN)
	{
		if (!rs2Opened)
		{
			confirmModal->setMessage("Still no connection to Rs2. Please check that the rs2 is (1) connected, (2) powerer and (3) not used by another application.");
			confirmModal->show();
		}
		ofLogVerbose("Rs2Projector") << "Confirm modal window is closed" ;
    }   
	else if (e.type == ofxModalEvent::CANCEL)
	{
		applicationState = APPLICATION_STATE_SETUP;
        ofLogVerbose("Rs2Projector") << "Modal cancel button pressed: Aborting" ;
		updateStatusGUI();
    }
	else if (e.type == ofxModalEvent::CONFIRM)
	{
		if (applicationState == APPLICATION_STATE_CALIBRATING)
		{
            if (waitingForFlattenSand)
			{
                waitingForFlattenSand = false;
            }  
			else if ((calibrationState == CALIBRATION_STATE_PROJ_RS2_AUTO_CALIBRATION || (calibrationState == CALIBRATION_STATE_FULL_AUTO_CALIBRATION && fullCalibState == FULL_CALIBRATION_STATE_AUTOCALIB))
                        && autoCalibState == AUTOCALIB_STATE_NEXT_POINT)
			{
                if (!upframe)
				{
                    upframe = true;
                }
            }
        }
        ofLogVerbose("Rs2Projector") << "Modal confirm button pressed" ;
    }
}

void Rs2Projector::onCalibModalEvent(ofxModalEvent e)
{
    if (e.type == ofxModalEvent::SHOWN)
	{
		ofLogVerbose("Rs2Projector") << "calib modal window is open";
    }  
	else if (e.type == ofxModalEvent::HIDDEN)
	{
		ofLogVerbose("Rs2Projector") << "calib modal window is closed";
	}
	else if (e.type == ofxModalEvent::CONFIRM)
	{
		applicationState = APPLICATION_STATE_SETUP;
        ofLogVerbose("Rs2Projector") << "Modal cancel button pressed: Aborting" ;
		updateStatusGUI();
    }
}

void Rs2Projector::saveCalibrationAndSettings()
{
	if (projRs2Calibrated)
	{
		if (kpt->saveCalibration("settings/calibration.xml"))
		{
			ofLogVerbose("Rs2Projector") << "update(): initialisation: Calibration saved ";
		}
		else {
			ofLogVerbose("Rs2Projector") << "update(): initialisation: Calibration could not be saved ";
		}
	}
	if (ROIcalibrated)
	{
		if (saveSettings())
		{
			ofLogVerbose("Rs2Projector") << "update(): initialisation: Settings saved ";
		}
		else {
			ofLogVerbose("Rs2Projector") << "update(): initialisation: Settings could not be saved ";
		}
	}
}

bool Rs2Projector::loadSettings(){
    string settingsFile = "settings/rs2ProjectorSettings.xml";
    
    ofXml xml;
    if (!xml.load(settingsFile))
        return false;
    xml.setTo("RS2SETTINGS");
    rs2ROI = xml.getValue<ofRectangle>("rs2ROI");
    basePlaneNormalBack = xml.getValue<ofVec3f>("basePlaneNormalBack");
    basePlaneNormal = basePlaneNormalBack;
    basePlaneOffsetBack = xml.getValue<ofVec3f>("basePlaneOffsetBack");
    basePlaneOffset = basePlaneOffsetBack;
    basePlaneEq = xml.getValue<ofVec4f>("basePlaneEq");
    maxOffsetBack = xml.getValue<float>("maxOffsetBack");
    maxOffset = maxOffsetBack;
    spatialFiltering = xml.getValue<bool>("spatialFiltering");
    followBigChanges = xml.getValue<bool>("followBigChanges");
    numAveragingSlots = xml.getValue<int>("numAveragingSlots");
	doInpainting = xml.getValue<bool>("OutlierInpainting", false);
	doFullFrameFiltering = xml.getValue<bool>("FullFrameFiltering", false);
    return true;
}

bool Rs2Projector::saveSettings()
{
    string settingsFile = "settings/rs2ProjectorSettings.xml";

    ofXml xml;
    xml.addChild("RS2SETTINGS");
    xml.setTo("RS2SETTINGS");
    xml.addValue("rs2ROI", rs2ROI);
    xml.addValue("basePlaneNormalBack", basePlaneNormalBack);
    xml.addValue("basePlaneOffsetBack", basePlaneOffsetBack);
    xml.addValue("basePlaneEq", basePlaneEq);
    xml.addValue("maxOffsetBack", maxOffsetBack);
    xml.addValue("spatialFiltering", spatialFiltering);
    xml.addValue("followBigChanges", followBigChanges);
    xml.addValue("numAveragingSlots", numAveragingSlots);
	xml.addValue("OutlierInpainting", doInpainting);
	xml.addValue("FullFrameFiltering", doFullFrameFiltering);
	xml.setToParent();
    return xml.save(settingsFile);
}

void Rs2Projector::ProcessChessBoardInput(ofxCvGrayscaleImage& image)
{
	CheckAndNormalizeRs2ROI();

	unsigned char *imgD = image.getPixels().getData();
	unsigned char minV = 255;
	unsigned char maxV = 0;

	// Find min and max values inside ROI
	for (int y = rs2ROI.getMinY(); y < rs2ROI.getMaxY(); y++)
	{
		for (int x = rs2ROI.getMinX(); x < rs2ROI.getMaxX(); x++)
		{
			int idx = y* image.width + x;
			unsigned char val = imgD[idx];

			if (val > maxV)
				maxV = val;
			if (val < minV)
				minV = val;
		}
	}
	std::cout << "Min " << (int)minV << " max " << (int)maxV << std::endl;
	double scale = 255.0 / (maxV - minV);

	for (int y = 0; y < image.height; y++)
	{
		for (int x = 0; x < image.width; x++)
		{
			int idx = y* image.width + x;
			unsigned char val = imgD[idx];
			double newVal = (val - minV) * scale;
			newVal = std::min(newVal, 255.0);
			newVal = std::max(newVal, 0.0);

			imgD[idx] = (unsigned char)newVal;
		}
	}
}

void Rs2Projector::CheckAndNormalizeRs2ROI()
{
	bool fixed = false;
	if (rs2ROI.x < 0)
	{
		fixed = true;
		rs2ROI.x = 0;
	}
	if (rs2ROI.y < 0)
	{
		fixed = true;
		rs2ROI.y = 0;
	}
	if (rs2ROI.x + rs2ROI.width >= rs2Res.x)
	{
		fixed = true;
		rs2ROI.width = rs2Res.x - 1 - rs2ROI.x;
	}
	if (rs2ROI.y + rs2ROI.height >= rs2Res.y)
	{
		fixed = true;
		rs2ROI.height = rs2Res.y - 1 - rs2ROI.y;
	}
	
	if (fixed)
		ofLogVerbose("Rs2Projector") << "CheckAndNormalizeRs2ROI(): Rs2 ROI fixed since it was out of bounds";
}

void Rs2Projector::SaveFilteredDepthImageDebug()
{
	std::string rawValOutKC = ofToDataPath(DebugFileOutDir+ "RawValsRs2Coords.txt");
	std::string rawValOutWC = ofToDataPath(DebugFileOutDir + "RawValsWorldCoords.txt");
	std::string rawValOutHM = ofToDataPath(DebugFileOutDir + "RawValsHM.txt");
	std::string BinOutName = DebugFileOutDir + "RawBinImg.png";
	std::string DepthOutName = DebugFileOutDir + "RawDepthImg.png";

	std::ofstream fostKC(rawValOutKC.c_str());
	std::ofstream fostWC(rawValOutWC.c_str());
	std::ofstream fostHM(rawValOutHM.c_str());

	ofxCvFloatImage temp;
	temp.setFromPixels(FilteredDepthImage.getFloatPixelsRef().getData(), rs2Res.x, rs2Res.y);
	temp.setNativeScale(FilteredDepthImage.getNativeScaleMin(), FilteredDepthImage.getNativeScaleMax());
	temp.convertToRange(0, 1);
	ofxCvGrayscaleImage temp2;
	temp2.setFromPixels(temp.getFloatPixelsRef());
	ofSaveImage(temp2.getPixels(), DepthOutName);

	float *imgData = FilteredDepthImage.getFloatPixelsRef().getData();

	ofxCvGrayscaleImage BinImg;
	BinImg.allocate(rs2Res.x, rs2Res.y);
	unsigned char *binData = BinImg.getPixels().getData();

	for (int y = 0; y < rs2Res.y; y++)
	{
		for (int x = 0; x < rs2Res.x; x++)
		{
			int IDX = y * rs2Res.x + x;
			double val = imgData[IDX];

			fostKC << val << std::endl;

			// Rs2 coords
			ofVec4f kc = ofVec4f(x, y, val, 1);

			// World coords
			ofVec4f wc = rs2WorldMatrix*kc*kc.z;
			fostWC << wc.x << " " << wc.y << " " << wc.z << std::endl;

			float H = elevationAtrs2Coord(x, y);
			fostHM << H << std::endl;

			unsigned char BinOut = H > 0;

			binData[IDX] = BinOut;
		}
	}

	ofSaveImage(BinImg.getPixels(), BinOutName);
}

bool Rs2Projector::getBinaryLandImage(ofxCvGrayscaleImage& BinImg)
{
	if (!rs2Opened)
		return false;

	float *imgData = FilteredDepthImage.getFloatPixelsRef().getData();

	BinImg.allocate(rs2Res.x, rs2Res.y);
	unsigned char *binData = BinImg.getPixels().getData();

	for (int y = 0; y < rs2Res.y; y++)
	{
		for (int x = 0; x < rs2Res.x; x++)
		{
			int IDX = y * rs2Res.x + x;
			double val = imgData[IDX];

			float H = elevationAtrs2Coord(x, y);

			unsigned char BinOut = 255 * (H > 0);

			binData[IDX] = BinOut;
		}
	}

	return true;
}


ofRectangle Rs2Projector::getProjectorActiveROI()
{
	ofRectangle projROI = ofRectangle(ofPoint(0, 0), ofPoint(projRes.x, projRes.y));
	return projROI;
}

void Rs2Projector::SaveFilteredDepthImage()
{
	std::string rawValOutKC = ofToDataPath(DebugFileOutDir + "RawValsRs2Coords.txt");
	std::string rawValOutWC = ofToDataPath(DebugFileOutDir + "RawValsWorldCoords.txt");
	std::string rawValOutHM = ofToDataPath(DebugFileOutDir + "RawValsHM.txt");
	std::string BinOutName  = DebugFileOutDir + "RawBinImg.png";
	std::string DepthOutName = DebugFileOutDir + "RawDepthImg.png";

	std::ofstream fostKC(rawValOutKC.c_str());
	std::ofstream fostWC(rawValOutWC.c_str());
	std::ofstream fostHM(rawValOutHM.c_str());

	ofxCvFloatImage temp;
	temp.setFromPixels(FilteredDepthImage.getFloatPixelsRef().getData(), rs2Res.x, rs2Res.y);
	temp.setNativeScale(FilteredDepthImage.getNativeScaleMin(), FilteredDepthImage.getNativeScaleMax());
	temp.convertToRange(0, 1);
	ofxCvGrayscaleImage temp2;
	temp2.setFromPixels(temp.getFloatPixelsRef());
	ofSaveImage(temp2.getPixels(), DepthOutName);

	float *imgData = FilteredDepthImage.getFloatPixelsRef().getData();

	ofxCvGrayscaleImage BinImg;
	BinImg.allocate(rs2Res.x, rs2Res.y);
	unsigned char *binData = BinImg.getPixels().getData();

	for (int y = 0; y < rs2Res.y; y++)
	{
		for (int x = 0; x < rs2Res.x; x++)
		{
			int IDX = y * rs2Res.x + x;
			double val = imgData[IDX];
			
			fostKC << val << std::endl;

			// Rs2 coords
			ofVec4f kc = ofVec4f(x, y, val, 1);

			// World coords
			ofVec4f wc = rs2WorldMatrix*kc*kc.z;
			fostWC << wc.x << " " << wc.y << " " << wc.z << std::endl;

			float H = elevationAtrs2Coord(x, y);
			fostHM << H << std::endl;

			unsigned char BinOut = H > 0;

			binData[IDX] = BinOut;
		}
	}

	ofSaveImage(BinImg.getPixels(), BinOutName);
}

void Rs2Projector::SaveRs2ColorImage()
{
	std::string ColourOutName = DebugFileOutDir + "RawColorImage.png";
	std::string MedianOutName = DebugFileOutDir + "TemporalFilteredImage.png";
	ofSaveImage(rs2ColorImage.getPixels(), ColourOutName);

	if (TemporalFrameFilter.isValid())
	{
		ofxCvGrayscaleImage tempImage;
//		tempImage.allocate(rs2ColorImage.width, rs2ColorImage.height);
		if (TemporalFilteringType == 0)
			tempImage.setFromPixels(TemporalFrameFilter.getMedianFilteredImage(), rs2ColorImage.width, rs2ColorImage.height);
		if (TemporalFilteringType == 1)
			tempImage.setFromPixels(TemporalFrameFilter.getAverageFilteredColImage(), rs2ColorImage.width, rs2ColorImage.height);
		ofSaveImage(tempImage.getPixels(), MedianOutName);
	}

}



