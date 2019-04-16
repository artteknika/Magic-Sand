/***********************************************************************
KinectGrabber - KinectGrabber takes care of the communication with
the kinect and the filtering of depth frame.
Copyright (c) 2016 Thomas Wolf

--- Adapted from FrameFilter of the Augmented Reality Sandbox
Copyright (c) 2012-2015 Oliver Kreylos

This file is part of the Magic Sand.

The Magic Sand is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Magic Sand is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
***********************************************************************/

#include "Rs2Grabber.h"
#include "ofConstants.h"

Rs2Grabber::Rs2Grabber()
:newFrame(true),
bufferInitiated(false),
rs2Opened(false)
{
}

Rs2Grabber::~Rs2Grabber(){
    //    stop();
    waitForThread(true);
}

/// Start the thread.
void Rs2Grabber::start(){
    startThread(true);
}

/// Signal the thread to stop.  After calling this method,
/// isThreadRunning() will return false and the while loop will stop
/// next time it has the chance to.
void Rs2Grabber::stop(){
    stopThread();
}

bool Rs2Grabber::setup(){
	// settings and defaults
	storedframes = 0;
	ROIAverageValue = 0;
	setToGlobalAvg = 0;
	setToLocalAvg = 0;
	doInPaint = 0;
	doFullFrameFiltering = false;

	rs2.init();
	rs2.setRegistration(true); // To have correspondance between RGB and depth images
	rs2.setUseTexture(false);
	width = rs2.getWidth();
	height = rs2.getHeight();

	rs2DepthImage.allocate(width, height, 1);
    filteredframe.allocate(width, height, 1);
    rs2ColorImage.allocate(width, height);
    rs2ColorImage.setUseTexture(false);
	return openRs2();
}

bool Rs2Grabber::openRs2() {
	rs2Opened = rs2.open();
	return rs2Opened;
}
void Rs2Grabber::setupFramefilter(int sgradFieldresolution, float newMaxOffset, ofRectangle ROI, bool sspatialFilter, bool sfollowBigChange, int snumAveragingSlots) {
    gradFieldresolution = sgradFieldresolution;
    ofLogVerbose("Rs2Grabber") << "setupFramefilter(): Gradient Field resolution: " << gradFieldresolution;
    gradFieldcols = width / gradFieldresolution;
    ofLogVerbose("Rs2Grabber") << "setupFramefilter(): Width: " << width << " Gradient Field Cols: " << gradFieldcols;
    gradFieldrows = height / gradFieldresolution;
    ofLogVerbose("Rs2Grabber") << "setupFramefilter(): Height: " << height << " Gradient Field Rows: " << gradFieldrows;
    
    spatialFilter = sspatialFilter;
    followBigChange = sfollowBigChange;
    numAveragingSlots = snumAveragingSlots;
    minNumSamples = (numAveragingSlots+1)/2;
    maxOffset = newMaxOffset;

    //Framefilter default parameters
    maxVariance = 4 ;
    hysteresis = 0.5f ;
    bigChange = 10.0f ;
    maxgradfield = 1000;
    initialValue = 4000;
    minInitFrame = 60;
    
    //Setup ROI
    setRs2ROI(ROI);
    
    //setting buffers
	initiateBuffers();
}

void Rs2Grabber::initiateBuffers(void){
	filteredframe.set(0);

    averagingBuffer=new float[numAveragingSlots*height*width];
    float* averagingBufferPtr=averagingBuffer;
    for(int i=0;i<numAveragingSlots;++i)
        for(unsigned int y=0;y<height;++y)
            for(unsigned int x=0;x<width;++x,++averagingBufferPtr)
                *averagingBufferPtr=initialValue;
    
    averagingSlotIndex=0;
    
    /* Initialize the statistics buffer: */
    statBuffer=new float[height*width*3];
    float* sbPtr=statBuffer;
    for(unsigned int y=0;y<height;++y)
        for(unsigned int x=0;x<width;++x)
            for(int i=0;i<3;++i,++sbPtr)
                *sbPtr=0.0;
    
    /* Initialize the valid buffer: */
    validBuffer=new float[height*width];
    float* vbPtr=validBuffer;
    for(unsigned int y=0;y<height;++y)
        for(unsigned int x=0;x<width;++x,++vbPtr)
            *vbPtr=initialValue;
    
    /* Initialize the gradient field buffer: */
    gradField = new ofVec2f[gradFieldcols*gradFieldrows];
    ofVec2f* gfPtr=gradField;
    for(unsigned int y=0;y<gradFieldrows;++y)
        for(unsigned int x=0;x<gradFieldcols;++x,++gfPtr)
            *gfPtr=ofVec2f(0);
    
    bufferInitiated = true;
    currentInitFrame = 0;
    firstImageReady = false;
}

void Rs2Grabber::resetBuffers(void){
    if (bufferInitiated){
        bufferInitiated = false;
        delete[] averagingBuffer;
        delete[] statBuffer;
        delete[] validBuffer;
        delete[] gradField;
    }
    initiateBuffers();
}

void Rs2Grabber::threadedFunction() {
	while(isThreadRunning()) {
        this->actionsLock.lock(); // Update the grabber state if needed
        for(auto & action : this->actions) {
            action(*this);
        }
        this->actions.clear();
        this->actionsLock.unlock();
        
        rs2.update();
        if(rs2.isFrameNew()){
            rs2DepthImage = rs2.getRawDepthPixels();
            filter();
            filteredframe.setImageType(OF_IMAGE_GRAYSCALE);
            updateGradientField();
			rs2ColorImage.setFromPixels(rs2.getPixels());
        }
        if (storedframes == 0)
        {
            filtered.send(std::move(filteredframe));
			gradient.send(std::move(gradField));
            colored.send(std::move(rs2ColorImage.getPixels()));
            lock();
            storedframes += 1;
            unlock();
        }
        
    }
    rs2.close();
    delete[] averagingBuffer;
    delete[] statBuffer;
    delete[] validBuffer;
    delete[] gradField;
}

void Rs2Grabber::performInThread(std::function<void(Rs2Grabber&)> action) {
    this->actionsLock.lock();
    this->actions.push_back(action);
    this->actionsLock.unlock();
}

void Rs2Grabber::depth_filtering(){
    const RawDepth* inputFramePtr = static_cast<const RawDepth*>(rs2DepthImage.getData());
    float* filteredFramePtr = filteredframe.getData();
    inputFramePtr += minY*width;  // We only scan rs2 ROI
    filteredFramePtr += minY*width;
    
    for (unsigned int y = minY; y < maxY; ++y)
    {
        inputFramePtr += minX;
        filteredFramePtr += minX;
        
        for (unsigned int x = minX; x < maxX; ++x, ++inputFramePtr, ++filteredFramePtr)
        {
            float newVal = static_cast<float>(*inputFramePtr);
            *filteredFramePtr = newVal;
        }
        inputFramePtr += width - maxX;
        filteredFramePtr += width - maxX;
    }
}

void Rs2Grabber::filter(){
	if (bufferInitiated)
    {
        const RawDepth* inputFramePtr = static_cast<const RawDepth*>(rs2DepthImage.getData());
        float* filteredFramePtr = filteredframe.getData();
        float* averagingBufferPtr = averagingBuffer+averagingSlotIndex*height*width;
        float* statBufferPtr = statBuffer;
        float* validBufferPtr = validBuffer;
        inputFramePtr += minY*width;  // We only scan rs2 ROI
        averagingBufferPtr += minY*width;
        statBufferPtr += minY*width*3;
        validBufferPtr += minY*width;
        filteredFramePtr += minY*width;

		for(unsigned int y=minY ; y<maxY ; ++y)
        {
            inputFramePtr += minX;
            averagingBufferPtr += minX;
            statBufferPtr += minX*3;
            validBufferPtr += minX;
            filteredFramePtr += minX;
            for(unsigned int x=minX ; x<maxX ; ++x,++inputFramePtr,++averagingBufferPtr,statBufferPtr+=3,++validBufferPtr,++filteredFramePtr)
            {
                float newVal = static_cast<float>(*inputFramePtr);
                float oldVal = *averagingBufferPtr;
                
				if(newVal > maxOffset)//we are under the ceiling plane
                {
                    *averagingBufferPtr = newVal; // Store the value
                    if (followBigChange && statBufferPtr[0] > 0){ // Follow big changes
                        float oldFiltered = statBufferPtr[1]/statBufferPtr[0]; // Compare newVal with average
                        if(oldFiltered-newVal >= bigChange || newVal-oldFiltered >= bigChange)
                        {
                            float* aaveragingBufferPtr;
                            for (int i = 0; i < numAveragingSlots; i++){ // update all averaging slots
                                aaveragingBufferPtr = averagingBuffer + i*height*width + y*width +x;
                                *aaveragingBufferPtr = newVal;
                            }
                            statBufferPtr[0] = numAveragingSlots; //Update statistics
                            statBufferPtr[1] = newVal*numAveragingSlots;
                            statBufferPtr[2] = newVal*newVal*numAveragingSlots;
                        }
                    }
                    // Update the pixel's statistics:
                    ++statBufferPtr[0]; // Number of valid samples
                    statBufferPtr[1] += newVal; // Sum of valid samples
                    statBufferPtr[2] += newVal*newVal; // Sum of squares of valid samples
                    
                    // Check if the previous value in the averaging buffer was not initiated
                    if(oldVal != initialValue)
                    {
                        --statBufferPtr[0]; // Number of valid samples
                        statBufferPtr[1] -= oldVal; // Sum of valid samples
                        statBufferPtr[2] -= oldVal * oldVal; // Sum of squares of valid samples
                    }
                }
                // Check if the pixel is "stable":
                if(statBufferPtr[0] >= minNumSamples &&
                   statBufferPtr[2]*statBufferPtr[0] <= maxVariance*statBufferPtr[0]*statBufferPtr[0] + statBufferPtr[1]*statBufferPtr[1])
                {
                    // Check if the new running mean is outside the previous value's envelope:
                    float newFiltered = statBufferPtr[1]/statBufferPtr[0];
                    if(abs(newFiltered-*validBufferPtr) >= hysteresis)
                    {
                        // Set the output pixel value to the depth-corrected running mean:
                        *filteredFramePtr = *validBufferPtr = newFiltered;
                    } else {
                        // Leave the pixel at its previous value:
                        *filteredFramePtr = *validBufferPtr;
                    }
                }
                *filteredFramePtr = *validBufferPtr;
			}
            inputFramePtr += width-maxX;
            averagingBufferPtr += width-maxX;
            statBufferPtr += (width-maxX)*3;
            validBufferPtr += width-maxX;
            filteredFramePtr += width-maxX;
        }
        // Go to the next averaging slot:
        if(++averagingSlotIndex==numAveragingSlots)
            averagingSlotIndex=0;
        
        if (!firstImageReady){
            currentInitFrame++;
            if(currentInitFrame > minInitFrame){
                firstImageReady = true;
                std::cout << "Ready Calibration" << std::endl;
            }
        }
        depth_filtering();
		if (doInPaint)
		{
			applySimpleOutlierInpainting();
		}

        // Apply a spatial filter if requested:
        if(spatialFilter)
        {
            applySpaceFilter();
        }
	}
}

void Rs2Grabber::setFullFrameFiltering(bool ff, ofRectangle ROI)
{
	doFullFrameFiltering = ff;
	if (ff)
	{
		setRs2ROI(ofRectangle(0, 0, width, height));
	}
	else 
	{
		setRs2ROI(ROI);
		float *data = filteredframe.getData();

		// Clear all pixels outside ROI
		for (unsigned int y = 0; y < height; y++)
		{
			for (unsigned int x = 0; x < width; x++)
			{
				if (y < minY || y >= maxY || x < minX || x >= maxX)
				{
					int idx = y * width + x;
					data[idx] = 0;
				}
			}
		}

	}
}

void Rs2Grabber::applySpaceFilter()
{
    for(int filterPass=0;filterPass< 20;++filterPass)
    {
		// Pointer to first pixel of ROI
		float *ptrOffset = filteredframe.getData() + minY * width + minX;

        // Low-pass filter the values in the ROI
		// First a horisontal pass
        for(unsigned int x = 0; x < width; x++)
        {
			// Pointer to current pixel
            float* colPtr = ptrOffset + x;
			float lastVal = *colPtr;

            // Top border pixels 
            *colPtr = (colPtr[0]*2.0f + colPtr[width]) / 3.0f;
            colPtr += width;
            
            // Filter the interior pixels in the column
            for(unsigned int y = minY+1; y < maxY-1; ++y, colPtr += width)
            {
				float nextLastVal = *colPtr;
                *colPtr=(lastVal + colPtr[0]*2.0f + colPtr[width])*0.25f;
				lastVal = nextLastVal; // To avoid using already updated pixels
            }
            
            // Filter the last pixel in the column: 
            *colPtr=(lastVal + colPtr[0] * 2.0f)/3.0f;
        }

		// then a vertical pass
        for(unsigned int y = 0; y < height; y++)
        {
			// Pointer to current pixel
			float* rowPtr = ptrOffset + y * width;
			
			// Filter the first pixel in the row: 
            float lastVal=*rowPtr;
            *rowPtr=(rowPtr[0]*2.0f + rowPtr[1]) / 3.0f;
            rowPtr++;
       
            // Filter the interior pixels in the row: 
            for(unsigned int x = minX+1; x < maxX-1; ++x,++rowPtr)
            {
                float nextLastVal=*rowPtr;
                *rowPtr=(lastVal+rowPtr[0]*2.0f+rowPtr[1])*0.25f;
                lastVal=nextLastVal;
            }
            
            // Filter the last pixel in the row: 
            *rowPtr=(lastVal+rowPtr[0]*2.0f)/3.0f;
        }
    }
}

void Rs2Grabber::updateGradientField()
{
    int ind = 0;
    float gx;
    float gy;
    int gvx, gvy;
    float lgth = 0;
    float* filteredFramePtr=filteredframe.getData();
    for(unsigned int y=0;y<gradFieldrows;++y) {
        for(unsigned int x=0;x<gradFieldcols;++x) {
            if (isInsideROI(x*gradFieldresolution, y*gradFieldresolution) && isInsideROI((x+1)*gradFieldresolution, (y+1)*gradFieldresolution) ){
                gx = 0;
                gvx = 0;
                gy = 0;
                gvy = 0;
                for (unsigned int i=0; i<gradFieldresolution; i++) {
                    ind = y*gradFieldresolution*width+i*width+x*gradFieldresolution;
                    if (filteredFramePtr[ind]!= 0 && filteredFramePtr[ind+gradFieldresolution-1]!=0){
                        gvx+=1;
                        gx+=filteredFramePtr[ind]-filteredFramePtr[ind+gradFieldresolution-1];
                    }
                    ind = y*gradFieldresolution*width+i+x*gradFieldresolution;
                    if (filteredFramePtr[ind]!= 0 && filteredFramePtr[ind+(gradFieldresolution-1)*width]!=0){
                        gvy+=1;
                        gy+=filteredFramePtr[ind]-filteredFramePtr[ind+(gradFieldresolution-1)*width];
                    }
                }
                if (gvx !=0 && gvy !=0)
                    gradField[y*gradFieldcols+x]=ofVec2f(gx/gradFieldresolution/gvx, gy/gradFieldresolution/gvy);
                if (gradField[y*gradFieldcols+x].length() > maxgradfield){
                    gradField[y*gradFieldcols+x].scale(maxgradfield);
                    lgth+=1;
                }
            } else {
                gradField[y*gradFieldcols+x] = ofVec2f(0);
            }
        }
    }
}


float Rs2Grabber::findInpaintValue(float *data, int x, int y)
{
	int sideLength = 5;

	// We do not search outside ROI
	int tminx = max(minX, x - sideLength);
	int tmaxx = min(maxX, x + sideLength);
	int tminy = max(minY, y - sideLength);
	int tmaxy = min(maxY, y + sideLength);

	int samples = 0;
	double sumval = 0;
	for (int y = tminy; y < tmaxy; y++)
	{
		for (int x = tminx; x < tmaxy; x++)
		{
			int idx = y * width + x;
			float val = data[idx];
			if (val != 0 && val != initialValue)
			{
				samples++;
				sumval += val;
			}
		}
	}
	// No valid samples found in neighboorhood
	if (samples == 0)
		return 0;

	return sumval / samples;
}

void Rs2Grabber::applySimpleOutlierInpainting()
{
	float *data = filteredframe.getData();

	// Estimate overall average inside ROI
	int samples = 0;
	ROIAverageValue = 0;
	for (unsigned int y = minY; y < maxY; y++)
	{
		for (unsigned int x = minX; x < maxX; x++)
		{
			int idx = y * width + x;
			float val = data[idx];
			if (val != 0 && val != initialValue)
			{
				samples++;
				ROIAverageValue += val;
			}
		}
	}
	// No valid samples found in ROI - strange situation
	if (samples == 0)
		ROIAverageValue = initialValue;
	
	ROIAverageValue /= samples;

	setToLocalAvg = 0;
	setToGlobalAvg = 0;
	// Filter ROI
	for (unsigned int y = max(0, minY-2); y < min((int)height, maxY+2); y++)
	{
		for (unsigned int x = max(0, minX-2); x < min((int)width, maxX+2); x++)
		{
			int idx = y * width + x;
			float val = data[idx];

			if (val == 0 || val == initialValue)
			{
				float newval = findInpaintValue(data, x, y);
				if (newval == 0)
				{
					newval = ROIAverageValue;
					setToGlobalAvg++;
				}
				else
				{

					setToLocalAvg++;
				}
				data[idx] = newval;
			}
		}
	}
}

bool Rs2Grabber::isInsideROI(int x, int y){
    if (x<minX||x>maxX||y<minY||y>maxY)
        return false;
    return true;
}

void Rs2Grabber::setRs2ROI(ofRectangle ROI){
	if (doFullFrameFiltering)
	{
		minX = 0;
		maxX = width;
		minY = 0;
		maxY = height;
	}
	else
	{ // we extend a bit beyond the border - to get data here as well due to shader issues
		minX = static_cast<int>(ROI.getMinX()) - 2;
		maxX = static_cast<int>(ROI.getMaxX()) + 2;
		minY = static_cast<int>(ROI.getMinY()) - 2;
		maxY = static_cast<int>(ROI.getMaxY()) + 2;
		
		minX = max(0, minX);
		maxX = min(maxX, (int)width);
		minY = max(0, minY);
		maxY = min(maxY, (int)height);
	}
    //ROIwidth = maxX-minX;
    //ROIheight = maxY-minY;
    resetBuffers();
}

void Rs2Grabber::setAveragingSlotsNumber(int snumAveragingSlots){
    if (bufferInitiated){
            bufferInitiated = false;
            delete[] averagingBuffer;
            delete[] statBuffer;
            delete[] validBuffer;
            delete[] gradField;
        }
    numAveragingSlots = snumAveragingSlots;
    minNumSamples=(numAveragingSlots+1)/2;
    initiateBuffers();
}

void Rs2Grabber::setGradFieldResolution(int sgradFieldresolution){
    if (bufferInitiated){
        bufferInitiated = false;
        delete[] averagingBuffer;
        delete[] statBuffer;
        delete[] validBuffer;
        delete[] gradField;
    }
    gradFieldresolution = sgradFieldresolution;
    initiateBuffers();
}

void Rs2Grabber::setFollowBigChange(bool newfollowBigChange){
    if (bufferInitiated){
        bufferInitiated = false;
        delete[] averagingBuffer;
        delete[] statBuffer;
        delete[] validBuffer;
        delete[] gradField;
    }
    followBigChange = newfollowBigChange;
    initiateBuffers();
}

ofVec3f Rs2Grabber::getStatBuffer(int x, int y){
    float* statBufferPtr = statBuffer+3*(x + y*width);
    return ofVec3f(statBufferPtr[0], statBufferPtr[1], statBufferPtr[2]);
}

float Rs2Grabber::getAveragingBuffer(int x, int y, int slotNum){
    float* averagingBufferPtr = averagingBuffer + slotNum*height*width + (x + y*width);
    return *averagingBufferPtr;
}

float Rs2Grabber::getValidBuffer(int x, int y){
    float* validBufferPtr = validBuffer + (x + y*width);
    return *validBufferPtr;
}

ofMatrix4x4 Rs2Grabber::getWorldMatrix() {
	auto mat = ofMatrix4x4();
	if (rs2Opened) {
		ofVec3f a = rs2.getWorldCoordinateAt(0, 0, 1);// Trick to access rs2 internal parameters without having to modify ofxRealSense2
		ofVec3f b = rs2.getWorldCoordinateAt(1, 1, 1);
		ofLogVerbose("rs2Grabber") << "getWorldMatrix(): Computing rs2 world matrix";
		mat = ofMatrix4x4(b.x - a.x, 0, 0, a.x,
			0, b.y - a.y, 0, a.y,
			0, 0, 0, 1,
			0, 0, 0, 1);
	}
	return mat;
}
