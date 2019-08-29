#include "ofxRealSense2.h"
#include "ofMain.h"


#include <librealsense2/rs.hpp>


#define OFX_KINECT_GRAVITY 9.80665

#define USE_INFRARED_AS_RGB FALSE
// context static
ofxRealSense2Context ofxRealSense2::realSense2Context;

//--------------------------------------------------------------------
ofxRealSense2::ofxRealSense2() {
	ofLogVerbose("ofxRealSense2") <<" creating ofxRealSense2";

	deviceId = -1;
	serial = "";

	bUseTexture = true;
	bGrabVideo = true;

	// set defaults
	bGrabberInited = false;

	bNeedsUpdateVideo = false;
	bNeedsUpdateDepth = false;
	bIsFrameNewVideo = false;
	bIsFrameNewDepth = false;

	bIsVideoInfrared = false;
	videoBytesPerPixel = 3;

	realSense2Device = NULL;

	targetTiltAngleDeg = 0;
	currentTiltAngleDeg = 0;
	bTiltNeedsApplying = false;

    currentLed = -1;
    bLedNeedsApplying = false;
	bHasMotorControl = false;

	lastDeviceId = -1;
	tryCount = 0;
	timeSinceOpen = 0;
	bGotData = false;

	bUseRegistration = false;
	bNearWhite = true;

	pixelFormat = OF_PIXELS_RGB;

	setDepthClipping();
}

//--------------------------------------------------------------------
ofxRealSense2::~ofxRealSense2() {
	close();
	clear();
}

//--------------------------------------------------------------------
bool ofxRealSense2::init(bool infrared, bool video, bool texture) {
	if(isConnected()) {
		ofLogWarning("ofxRealSense2") << "init(): do not call init while ofxRealSense2 is running!";
		return false;
	}

	clear();

	bIsVideoInfrared = infrared;
	bGrabVideo = video;
	videoBytesPerPixel = infrared?1:3;
	if(infrared){
		pixelFormat = OF_PIXELS_GRAY;
	}
    std::cout << "infra red :" << infrared << std::endl;
	bUseTexture = texture;

	// allocate
	depthPixelsRaw.allocate(width, height, 1);
	depthPixelsRawBack.allocate(width, height, 1);
	depthPixelsRawIntra.allocate(width, height, 1);

    //We have to do this as freenect has 488 pixels for the IR image height.
    //Instead of having slightly different sizes depending on capture we will crop the last 8 rows of pixels which are empty.
    int videoHeight = height;
    if( infrared ){
        videoHeight = 488;
    }

	videoPixels.allocate(width, height, videoBytesPerPixel);
	videoPixelsBack.allocate(width, videoHeight, videoBytesPerPixel);
	videoPixelsIntra.allocate(width, videoHeight, videoBytesPerPixel);

	depthPixels.allocate(width, height, 1);
	distancePixels.allocate(width, height, 1);

	 // set
	depthPixelsRaw.set(0);
	depthPixelsRawBack.set(0);

	videoPixels.set(0);
	videoPixelsBack.set(0);

	depthPixels.set(0);
	distancePixels.set(0);

	if(bUseTexture) {
		depthTex.allocate(depthPixels);
		videoTex.allocate(videoPixels);
	}

	if(!realSense2Context.isInited()) {
		if(!realSense2Context.init()) {
			return false;
		}
	}

	bGrabberInited = true;

	return bGrabberInited;
}

//---------------------------------------------------------------------------
void ofxRealSense2::clear() {
	if(isConnected()) {
		ofLogWarning("ofxRealSense2") << "clear(): do not call clear while ofxRealSense2 is running!";
		return;
	}

	depthPixelsRaw.clear();
	depthPixelsRawBack.clear();

	videoPixels.clear();
	videoPixelsBack.clear();

	depthPixels.clear();
	distancePixels.clear();

	depthTex.clear();
	videoTex.clear();

	bGrabberInited = false;
}

//--------------------------------------------------------------------
void ofxRealSense2::setRegistration(bool bUseRegistration) {
	this->bUseRegistration = bUseRegistration;
}

//--------------------------------------------------------------------
bool ofxRealSense2::open(int deviceIndex) {
	if(!bGrabberInited) {
		ofLogWarning("ofxRealSense2") << "open(): cannot open, init not called";
		return false;
	}

    //we need to find the device id from the device list index.
    //as the first device id could be 1 and not 0 when the device list is sorted by serial
    int deviceIDFromIndex = -1;
    if( deviceIndex > -1 ){
        deviceIDFromIndex = realSense2Context.getDeviceId(deviceIndex);
    }

	if(!realSense2Context.open(*this, deviceIDFromIndex)) {
		return false;
	}

	if(serial == "0000000000000000") {
        bHasMotorControl = false;
        //if we do motor control via the audio device ( ie: 1473 or k4w ) and we have firmware uploaded
        //then we can do motor stuff! :)
    }
	else {
		bHasMotorControl = true;
	}
    /*
    // TODO
    if( kinectDevice->motor_control_with_audio_enabled ){
     bHasMotorControl = true;
    }else{
     ofLogVerbose("ofxRealSense2") << "open(): device " << deviceId << " does not have motor control";
    }
    */

	lastDeviceId = deviceId;
	timeSinceOpen = ofGetElapsedTimef();
	bGotData = false;

    // TODO
    /*
     freenect_set_user(kinectDevice, this);
     freenect_set_depth_buffer(kinectDevice, depthPixelsRawBack.getData());
     freenect_set_video_buffer(kinectDevice, videoPixelsBack.getData());
     freenect_set_depth_callback(kinectDevice, &grabDepthFrame);
     freenect_set_video_callback(kinectDevice, &grabVideoFrame);
     */

	is_d435 = false;
	rs2::context context;
	auto devices = context.query_devices();
	for (auto device : devices) {
		if (this->serial == device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)) {
			std::string product_id = device.get_info(RS2_CAMERA_INFO_PRODUCT_ID);
			if ("0B07" == product_id) {
				is_d435 = true;
			}
			break;
		}
	}


	rs2::pipeline& pipe = pipeline;
	rs2::config config;
	int width = this->DEPTH_X_RES;
	int height = this->DEPTH_Y_RES;
	int depth_raw_width = is_d435 ? 1280 : this->DEPTH_X_RES;
	int depth_raw_height = is_d435 ? 720 : this->DEPTH_Y_RES;
	int fps = 30;
	auto serial = this->serial;
	config.enable_device(serial);
	if (bGrabVideo) {
#if USE_INFRARED_AS_RGB
		config.enable_stream(RS2_STREAM_INFRARED, width, height, RS2_FORMAT_Y8, fps);
#else
		config.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_ANY, fps);
#endif
	}
	config.enable_stream(RS2_STREAM_DEPTH, depth_raw_width, depth_raw_height, RS2_FORMAT_ANY, fps);

	if (true)
	{
		pipe.start(config);

		auto device = pipe.get_active_profile().get_device();
		auto product_id = device.get_info(RS2_CAMERA_INFO_PRODUCT_ID);


		auto frames = pipe.wait_for_frames();

		rgb_intr = frames.get_color_frame().get_profile().as<rs2::video_stream_profile>().get_intrinsics(); // Calibration data
		depthr_intr = frames.get_depth_frame().get_profile().as<rs2::video_stream_profile>().get_intrinsics(); // Calibration data


	}

	startThread(); // blocking, not verbose

	return true;
}

//--------------------------------------------------------------------
bool ofxRealSense2::open(string serial) {
    if(!bGrabberInited) {
        ofLogVerbose("ofxKinect") << "open(): cannot open, init not called";
        return false;
    }
    if(!realSense2Context.open(*this, serial)) {
        return false;
    }
    if(serial == "0000000000000000") {
        bHasMotorControl = false;
        //if we do motor control via the audio device ( ie: 1473 or k4w ) and we have firmware uploaded
        //then we can do motor stuff! :)
        /*
        // TODO
        if( kinectDevice->motor_control_with_audio_enabled ){
            bHasMotorControl = true;
        }else{
            ofLogVerbose("ofxKinect") << "open(): device " << deviceId << " does not have motor control";
        }
        */
    }
    else {
        bHasMotorControl = true;
    }
    lastDeviceId = deviceId;
    timeSinceOpen = ofGetElapsedTimef();
    bGotData = false;
    startThread(); // blocking, not verbose
    return true;
}

//---------------------------------------------------------------------------
void ofxRealSense2::close() {
	if(isThreadRunning()) {
		stopThread();
		ofSleepMillis(10);
		waitForThread(false);
	}

	deviceId = -1;
	serial = "";
	bIsFrameNewVideo = false;
	bNeedsUpdateVideo = false;
	bIsFrameNewDepth = false;
	bNeedsUpdateDepth = false;
}

//---------------------------------------------------------------------------
bool ofxRealSense2::isConnected() const{
	return isThreadRunning();
}

//---------------------------------------------------------------------------
bool ofxRealSense2::isInitialized() const{
	return realSense2Context.isInited();
}

//--------------------------------------------------------------------
bool ofxRealSense2::isFrameNew()  const{
	return isFrameNewVideo() || isFrameNewDepth();
}

//--------------------------------------------------------------------
bool ofxRealSense2::isFrameNewVideo() const{
	return bIsFrameNewVideo;
}

//--------------------------------------------------------------------
bool ofxRealSense2::isFrameNewDepth() const{
	return bIsFrameNewDepth;
}

//--------------------------------------------------------------------
bool ofxRealSense2::setPixelFormat(ofPixelFormat pixelFormat){
	if(!bIsVideoInfrared && pixelFormat==OF_PIXELS_RGB){
		return true;
	}else if(pixelFormat == OF_PIXELS_GRAY){
		return true;
	}else{
		return false;
	}
}

//--------------------------------------------------------------------
ofPixelFormat ofxRealSense2::getPixelFormat() const{
	if(!bIsVideoInfrared){
		return OF_PIXELS_RGB;
	}else{
		return OF_PIXELS_GRAY;
	}
}

//----------------------------------------------------------
void ofxRealSense2::update() {
	if(!bGrabberInited) {
		return;
	}

	if(!bNeedsUpdateVideo && !bNeedsUpdateDepth && !bGotData && tryCount < 5 && ofGetElapsedTimef() - timeSinceOpen > 20.0 ){
		close();
		ofLogWarning("ofxRealSense2") << "update(): device " << lastDeviceId << " isn't delivering data, reconnecting tries: " << tryCount+1;
		realSense2Context.buildDeviceList();
		open(lastDeviceId);
		tryCount++;
		timeSinceOpen = ofGetElapsedTimef();
		return;
	}

	if(bNeedsUpdateVideo){
		bIsFrameNewVideo = true;
		bGotData = true;
		tryCount = 0;
		if(this->lock()) {
            if( videoPixels.getHeight() == videoPixelsIntra.getHeight() ){
                swap(videoPixels,videoPixelsIntra);
            }else{
				int minimumSize = MIN(videoPixels.size(), videoPixelsIntra.size());
				memcpy(videoPixels.getData(), videoPixelsIntra.getData(), minimumSize);
            }
			bNeedsUpdateVideo = false;
			this->unlock();
		}

		if(bUseTexture) {
			videoTex.loadData(videoPixels);
		}
	} else {
		bIsFrameNewVideo = false;
	}

	if(bNeedsUpdateDepth){
		bIsFrameNewDepth = true;
		bGotData = true;
		tryCount = 0;
		if(this->lock()) {
			swap(depthPixelsRaw, depthPixelsRawIntra);
			bNeedsUpdateDepth = false;
			this->unlock();

			updateDepthPixels();
		}

		if(bUseTexture) {
			depthTex.loadData(depthPixels);
		}
	} else {
		bIsFrameNewDepth = false;
	}

}

//------------------------------------
float ofxRealSense2::getDistanceAt(int x, int y)  const{
	return depthPixelsRaw[y * width + x];
}

//------------------------------------
float ofxRealSense2::getDistanceAt(const ofPoint & p)  const{
	return getDistanceAt(p.x, p.y);
}

//------------------------------------
ofVec3f ofxRealSense2::getWorldCoordinateAt(int x, int y)  const{
	return getWorldCoordinateAt(x, y, getDistanceAt(x, y));
}

//------------------------------------
ofVec3f ofxRealSense2::getWorldCoordinateAt(float cx, float cy, float wz)  const{
	double wx, wy;
#if USE_INFRARED_AS_RGB
	// Stereo Module
	double ref_pix_size = depthr_intr.ppx / (depthr_intr.width / 2);
	double ref_distance = depthr_intr.fx;
#else
	// RGB CAmera

	double ref_pix_size = rgb_intr.ppx / (rgb_intr.width / 2);
	double ref_distance = rgb_intr.fx;
#endif

	// Kincet v1
	ref_pix_size = 0.10419999808073044;
	ref_distance = 120;


	double factor = 2 * ref_pix_size * wz / ref_distance;
	//factor = 1.0;
	wx = (double)(cx - this->DEPTH_X_RES / 2) * factor;
	wy = (double)(cy - this->DEPTH_Y_RES / 2) * factor;


	return ofVec3f(wx, wy, wz);
}

//------------------------------------
float ofxRealSense2::getSensorEmitterDistance()  const{
	// TODO
	return 0.0f;
}

//------------------------------------
float ofxRealSense2::getSensorCameraDistance()  const{
	// TODO
	return 0.0f;
}

//------------------------------------
float ofxRealSense2::getZeroPlanePixelSize()  const{
	// TODO
	return 0.0f;
}

//------------------------------------
float ofxRealSense2::getZeroPlaneDistance()  const{
	// TODO
	return 0.0f;
}

ofColor ofxRealSense2::getColorAt(const ofPoint & p)  const{
    return getColorAt(p.x, p.y);
}

//------------------------------------
ofColor ofxRealSense2::getColorAt(int x, int y)  const{
	int index = (y * width + x) * videoBytesPerPixel;
	ofColor c;
	c.r = videoPixels[index + 0];
	c.g = videoPixels[index + (videoBytesPerPixel-1)/2];
	c.b = videoPixels[index + (videoBytesPerPixel-1)];
	c.a = 255;

	return c;
}

ofPixels & ofxRealSense2::getPixels(){
	return videoPixels;
}

ofPixels & ofxRealSense2::getDepthPixels(){
	return depthPixels;
}

ofShortPixels & ofxRealSense2::getRawDepthPixels(){
	return depthPixelsRaw;
}

ofFloatPixels & ofxRealSense2::getDistancePixels(){
	return distancePixels;
}

const ofPixels & ofxRealSense2::getPixels() const{
	return videoPixels;
}

const ofPixels & ofxRealSense2::getDepthPixels() const{
	return depthPixels;
}

const ofShortPixels & ofxRealSense2::getRawDepthPixels() const{
	return depthPixelsRaw;
}

const ofFloatPixels & ofxRealSense2::getDistancePixels() const{
	return distancePixels;
}

//------------------------------------
ofTexture& ofxRealSense2::getTexture(){
	if(!videoTex.isAllocated()){
		ofLogWarning("ofxRealSense2") << "getTexture(): device " << deviceId << " video texture not allocated";
	}
	return videoTex;
}

//---------------------------------------------------------------------------
ofTexture& ofxRealSense2::getDepthTexture(){
	if(!depthTex.isAllocated()){
		ofLogWarning("ofxRealSense2") << "getDepthTexture(): device " << deviceId << " depth texture not allocated";
	}
	return depthTex;
}

//------------------------------------
const ofTexture& ofxRealSense2::getTexture() const{
	if(!videoTex.isAllocated()){
		ofLogWarning("ofxRealSense2") << "getTexture(): device " << deviceId << " video texture not allocated";
	}
	return videoTex;
}

//---------------------------------------------------------------------------
const ofTexture& ofxRealSense2::getDepthTexture() const{
	if(!depthTex.isAllocated()){
		ofLogWarning("ofxRealSense2") << "getDepthTexture(): device " << deviceId << " depth texture not allocated";
	}
	return depthTex;
}

//------------------------------------
const ofTexture& ofxRealSense2::getTextureReference() const{
	return getTexture();
}

//---------------------------------------------------------------------------
const ofTexture& ofxRealSense2::getDepthTextureReference() const{
	return getDepthTexture();
}

//---------------------------------------------------------------------------
void ofxRealSense2::enableDepthNearValueWhite(bool bEnabled) {
	bNearWhite = bEnabled;
	updateDepthLookupTable();
}

//---------------------------------------------------------------------------
bool ofxRealSense2::isDepthNearValueWhite() const{
	return bNearWhite;
}

//---------------------------------------------------------------------------
void ofxRealSense2::setDepthClipping(float nearClip, float farClip) {
	nearClipping = nearClip;
	farClipping = farClip;
	updateDepthLookupTable();
}

//---------------------------------------------------------------------------
float ofxRealSense2::getNearClipping() const{
    return nearClipping;
}

//---------------------------------------------------------------------------
float ofxRealSense2::getFarClipping() const{
    return farClipping;
}

//--------------------------------------------------------------------
bool ofxRealSense2::hasAccelControl() const{
	return bHasMotorControl; // depends on motor for now
}

bool ofxRealSense2::hasCamTiltControl() const{
	return bHasMotorControl; // depends on motor for now
}

bool ofxRealSense2::hasLedControl() const{
	return bHasMotorControl; // depends on motor for now
}

//---------------------------------------------------------------------------
ofPoint ofxRealSense2::getRawAccel() const{
	return rawAccel;
}

//---------------------------------------------------------------------------
ofPoint ofxRealSense2::getMksAccel() const{
	return mksAccel;
}

//---------------------------------------------------------------------------
float ofxRealSense2::getAccelPitch() const{
	return ofRadToDeg(asin(getMksAccel().z/OFX_KINECT_GRAVITY));
}

//---------------------------------------------------------------------------
float ofxRealSense2::getAccelRoll() const{
	return ofRadToDeg(asin(getMksAccel().x/OFX_KINECT_GRAVITY));
}

// we update the value here, but apply it in kinect thread
//--------------------------------------------------------------------
bool ofxRealSense2::setCameraTiltAngle(float angleInDegrees) {

	if(!hasCamTiltControl() || !bGrabberInited) {
		return false;
	}

	targetTiltAngleDeg = ofClamp(angleInDegrees, -30, 30);
	bTiltNeedsApplying = true;

	return true;
}

//--------------------------------------------------------------------
float ofxRealSense2::getTargetCameraTiltAngle() const{
	return targetTiltAngleDeg;
}

float ofxRealSense2::getCurrentCameraTiltAngle() const{
	return currentTiltAngleDeg;
}

//--------------------------------------------------------------------

/*
void ofxRealSense2::setLed(ofxRealSense2::LedMode mode) {
    if(!hasLedControl() || mode == currentLed) {
        return;
    }
    bLedNeedsApplying = true;
    currentLed = mode;
}
*/

//------------------------------------
void ofxRealSense2::setUseTexture(bool bUse){
	bUseTexture = bUse;
}

//------------------------------------
bool ofxRealSense2::isUsingTexture() const{
	return bUseTexture;
}

//----------------------------------------------------------
void ofxRealSense2::draw(float _x, float _y, float _w, float _h) const{
	if(bUseTexture && bGrabVideo) {
		videoTex.draw(_x, _y, _w, _h);
	}
}

//----------------------------------------------------------
void ofxRealSense2::draw(float _x, float _y) const{
	draw(_x, _y, (float)width, (float)height);
}

//----------------------------------------------------------
void ofxRealSense2::draw(const ofPoint & point) const{
	draw(point.x, point.y);
}

//----------------------------------------------------------
void ofxRealSense2::draw(const ofRectangle & rect) const{
	draw(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
void ofxRealSense2::drawDepth(float _x, float _y, float _w, float _h) const{
	if(bUseTexture) {
		depthTex.draw(_x, _y, _w, _h);
	}
}

//---------------------------------------------------------------------------
void ofxRealSense2::drawDepth(float _x, float _y) const{
	drawDepth(_x, _y, (float)width, (float)height);
}

//----------------------------------------------------------
void ofxRealSense2::drawDepth(const ofPoint & point) const{
	drawDepth(point.x, point.y);
}

//----------------------------------------------------------
void ofxRealSense2::drawDepth(const ofRectangle & rect) const{
	drawDepth(rect.x, rect.y, rect.width, rect.height);
}

//---------------------------------------------------------------------------
int ofxRealSense2::getDeviceId() const{
	return deviceId;
}

//---------------------------------------------------------------------------
string ofxRealSense2::getSerial() const{
	return serial;
}

//----------------------------------------------------------
float ofxRealSense2::getHeight() const{
	return (float) height;
}

//---------------------------------------------------------------------------
float ofxRealSense2::getWidth() const{
	return (float) width;
}

//----------------------------------------------------------
void ofxRealSense2::listDevices() {
	realSense2Context.listDevices();
}

//---------------------------------------------------------------------------
int ofxRealSense2::numTotalDevices() {
	return realSense2Context.numTotal();
}

//---------------------------------------------------------------------------
int ofxRealSense2::numAvailableDevices() {
	return realSense2Context.numAvailable();
}

//---------------------------------------------------------------------------
int ofxRealSense2::numConnectedDevices() {
	return realSense2Context.numConnected();
}

//---------------------------------------------------------------------------
bool ofxRealSense2::isDeviceConnected(int id) {
	return realSense2Context.isConnected(id);
}

//---------------------------------------------------------------------------
bool ofxRealSense2::isDeviceConnected(string serial) {
	return realSense2Context.isConnected(serial);
}

//---------------------------------------------------------------------------
int ofxRealSense2::nextAvailableId() {
	return realSense2Context.nextAvailableId();
}

//---------------------------------------------------------------------------
string ofxRealSense2::nextAvailableSerial() {
	return realSense2Context.nextAvailableSerial();
}

/* ***** PRIVATE ***** */

//---------------------------------------------------------------------------
void ofxRealSense2::updateDepthLookupTable() {
	unsigned char nearColor = bNearWhite ? 255 : 0; ;// bNearWhite ? 255 : 0;
	unsigned char farColor = bNearWhite ? 0 : 255; //bNearWhite ? 0 : 255;
	unsigned int maxDepthLevels =  10001;
								//65536;//
	depthLookupTable.resize(maxDepthLevels);
	depthLookupTable[0] = 0;
	for(unsigned int i = 1; i < maxDepthLevels; i++) {
		depthLookupTable[i] = ofMap(i, nearClipping, farClipping, nearColor, farColor, true);
	}
}

//----------------------------------------------------------
void ofxRealSense2::updateDepthPixels() {
	int n = width * height;
	for(int i = 0; i < n; i++) {
		distancePixels[i] = depthPixelsRaw[i];
	}

	for(int i = 0; i < n; i++) {

		unsigned int rawDepth = depthPixelsRaw.getPixels()[i];
		if (rawDepth > 2000) rawDepth = 0;
		depthPixels[i] = depthLookupTable[rawDepth];
	}
}


//---------------------------------------------------------------------------
void ofxRealSense2::threadedFunction(){

#if FALSE
	rs2::pipeline pipe;

	rs2::config config;
	int width = this->DEPTH_X_RES;
	int height = this->DEPTH_Y_RES;
	int fps = 30;
    int depth_fps = 30;
	config.enable_device(this->serial);

	if (bGrabVideo) {
#if USE_INFRARED_AS_RGB
        config.enable_stream(RS2_STREAM_INFRARED, width, height, RS2_FORMAT_Y8, depth_fps);
#else
		config.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_ANY, fps);
#endif
	}
	config.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_ANY, depth_fps);



	//pipe.get_active_profile().get_stream(RS2_STREAM_DEPTH).get_extrinsics_to();//.get.as<rs2::video_stream_profile>().get_intrinsics();

	pipe.start(config);
	ofLogVerbose("ofxRealSense2") << "device " << deviceId << " " << serial << " connection opened";


	auto sensors = pipe.get_active_profile().get_device().query_sensors();
	//auto depth_scale = rs2_get_depth_scale(sensors.at(0).get().get(), nullptr);
#endif

	while (isThreadRunning())
	{

		rs2::frameset frameset;
		if (pipeline.poll_for_frames(&frameset)) {
			for (int i = 0; i < frameset.size(); i++)
			{
				rs2::frame new_frame = frameset[i];
				auto stream_type = new_frame.get_profile().stream_type();

				if (true)
				{

					switch (stream_type) {
#if USE_INFRARED_AS_RGB
					case RS2_STREAM_INFRARED:
					{
						this->lock();

						uint8_t* dst = videoPixels.getData();
						const uint8_t* src = reinterpret_cast<const uint8_t*>(new_frame.get_data());

						for (int i = 0; i < this->DEPTH_X_RES * this->DEPTH_Y_RES; i++) {

							unsigned char v = *src;
							if (v > 255) v = 255;
							dst[0] = v;
							dst[1] = v;
							dst[2] = v;
							src++;
							dst += videoPixels.getBytesPerPixel();
						}

						this->bNeedsUpdateVideo = true;

						this->unlock();
					}
					break;
#else
					case RS2_STREAM_COLOR:
						this->lock();

						memcpy(videoPixels.getData(), new_frame.get_data(), this->DEPTH_X_RES * this->DEPTH_Y_RES * 3);
						this->bNeedsUpdateVideo = true;

						this->unlock();
						break;
#endif

						case RS2_STREAM_DEPTH:
						{
							this->lock();

							unsigned char* buff = (unsigned char*)new_frame.get_data();

							// noise reduction
                            rs2::hole_filling_filter hole_filling;
                            new_frame = hole_filling.process(new_frame);

							if (is_d435)
							{
								unsigned short* dst = depthPixelsRaw.getData();
								const uint8_t* src = reinterpret_cast<const uint8_t*>(new_frame.get_data());
								src += (1280 * 2) * (720-this->DEPTH_Y_RES)/2;
								src += (1280 - this->DEPTH_X_RES)/2 * 2;
								for (int i = 0; i < this->DEPTH_Y_RES; i++) {
									memcpy(dst, src, this->DEPTH_X_RES * 2);
									src += 1280*2;
									dst += this->DEPTH_X_RES;
								}
							}
							else
							{
								memcpy(depthPixelsRaw.getData(), new_frame.get_data(), this->DEPTH_X_RES * this->DEPTH_Y_RES * 2);
							}

							bNeedsUpdateDepth = true;

							this->unlock();
						}
							break;
					}

				}
				else
				{
					// デプス画像確認用のテストコード
					switch (stream_type) {
					case RS2_STREAM_COLOR:
						break;
					case RS2_STREAM_DEPTH:
						this->lock();
						bNeedsUpdateVideo = true;
							uint8_t* dst = videoPixels.getData();
							const uint8_t* src = reinterpret_cast<const uint8_t*>(new_frame.get_data());
							//const uint16_t* src = reinterpret_cast<const uint16_t*>(new_frame.get_data());
							auto format = new_frame.get_profile().format();

							 int max_depth = 0;
							for (int i = 0; i < this->DEPTH_X_RES * this->DEPTH_Y_RES; i++) {
								unsigned int d = ((256 * src[1] + src[0]));
								if (max_depth < d && d < 12500/*d != 65535*/) max_depth = d;
								log(d);
								unsigned int v = (255.0/3.0) * log( d/200.0);// (16 * (1 * 256 * src[1] + src[0])) / 256;// / 256;// src[1];// ((src[0] << 8) /*+ src[1]*/) / 256;
								if (v > 255) v = 255;
								dst[0] = v;
								dst[1] = v;
								dst[2] = v;
								src += 2;
								//src += 1;
								dst += videoPixels.getBytesPerPixel();
							}

							ofLogVerbose("ofxRealSense2") << "max depth= " << max_depth;
						this->unlock();
						break;
					}
				}

			}
		}

	}
#if FALSE
	pipe.stop();
#endif
	ofLogVerbose("ofxRealSense2") << "device " << deviceId << " connection closed";

}

//---------------------------------------------------------------------------
// ofxRealSense2Context
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
ofxRealSense2Context::ofxRealSense2Context() {
	bInited = false;
	rs2Context = NULL;
}
ofxRealSense2Context::~ofxRealSense2Context() {
	closeAll();
	clear();
}

//---------------------------------------------------------------------------
static bool sortRealSensePairs(ofxRealSense2Context::RealSensePair A, ofxRealSense2Context::RealSensePair B){
	return A.serial < B.serial;
}

//---------------------------------------------------------------------------
bool ofxRealSense2Context::init() {
	rs2Context = new  rs2::context();
	if(rs2Context == NULL) {
		ofLogError("ofxRealSense2") << "init(): new rs2::context failed";
		bInited = false;
		return false;
	}

	bInited = true;
	ofLogVerbose("ofxRealSense2") << "context inited";

	buildDeviceList();
	listDevices(true);

	return true;
}

//---------------------------------------------------------------------------
void ofxRealSense2Context::clear() {
	if(isInited() && numConnected() < 1) {
		//freenect_shutdown(kinectContext);
		rs2Context = NULL;
		bInited = false;
		ofLogVerbose("ofxRealSense2") << "context cleared";
	}
}

bool ofxRealSense2Context::isInited() {
	return bInited;
}

bool ofxRealSense2Context::open(ofxRealSense2& realSense2, int id) {

	// rebuild if necessary (aka new kinects plugged in)
	buildDeviceList();

	if(numConnected() >= numTotal()) {
		ofLogWarning("ofxRealSense2") << "no available devices found";
		return false;
	}

	// is the id available?
	if(id < 0) {
		id = nextAvailableId();
	}
	if(isConnected(id)) {
		ofLogWarning("ofxRealSense2") << "device " << id << " already connected";
		return false;
	}

    // open and add to vector
    // TODO
    /*
    if(freenect_open_device(kinectContext, &kinect.kinectDevice, id) < 0) {
        ofLogError("ofxKinect") << "could not open device " <<  id;
        return false;
    }
    */
    /*
    rs2::config config;
    int width = 640;
    int height = 480;
    int fps = 30;
    auto serial = deviceList[getDeviceIndex(id)].serial;
    config.enable_device(serial);
    realSense2.pipe.start(config);
    */
    //realSense2.realSense2Device = deviceList[getDeviceIndex(id)].serial;

	realsenses.insert(pair<int, ofxRealSense2*>(id, &realSense2));

	// set RealSense id & serial from bus id
	realSense2.deviceId = id;
	realSense2.serial = deviceList[getDeviceIndex(id)].serial;

	return true;
}

bool ofxRealSense2Context::open(ofxRealSense2& realSense2, string serial) {

	// rebuild if necessary (aka new kinects plugged in)
	buildDeviceList();

	if(numConnected() >= numTotal()) {
		ofLogWarning("ofxRealSense2") << "no available devices found";
		return false;
	}

	// is the serial available?
	if(isConnected(serial)) {
		ofLogWarning("ofxRealSense2") << "device " << serial << " already connected";
		return false;
	}
    // open and add to vector
    /*
    // TODO
    if(freenect_open_device_by_camera_serial(kinectContext, &kinect.kinectDevice, serial.c_str()) < 0) {
        ofLogError("ofxKinect") << "could not open device " << serial;
        return false;
    }
     */
	int index = getDeviceIndex(serial);
	realsenses.insert(pair<int, ofxRealSense2*>(deviceList[index].id, &realSense2));
	realSense2.deviceId = deviceList[index].id;
	realSense2.serial = serial;

	return true;
}

void ofxRealSense2Context::close(ofxRealSense2& realSense2) {

	// check if it's already closed
	int id = -1;
	std::map<int, ofxRealSense2*>::iterator iter;
	for(iter = realsenses.begin(); iter != realsenses.end(); ++iter) {
		if(iter->second == &realSense2) {
			id = iter->first;
			break;
		}
	}
	if(id == -1)
		return;

	// remove connected device and close
	iter = realsenses.find(id);
	if(iter != realsenses.end()) {
		realsenses.erase(iter);
	}
}

void ofxRealSense2Context::closeAll() {
	// make copy of map to avoid invalidating iter when calling close()
	std::map<int, ofxRealSense2*> realsensesCopy(realsenses);
    std::map<int, ofxRealSense2*>::iterator iter;
    for(iter = realsensesCopy.begin(); iter != realsensesCopy.end(); ++iter) {
        iter->second->close();
    }
}

//---------------------------------------------------------------------------
void ofxRealSense2Context::buildDeviceList() {

	deviceList.clear();

	int index = 0;
	for (auto device : rs2Context->query_devices())
	{
		RealSensePair kp;
		kp.id = index;
		kp.serial = device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
		deviceList.push_back(kp);
		index++;
	}

	// sort devices by serial number
	sort(deviceList.begin(), deviceList.end(), sortRealSensePairs);
}

void ofxRealSense2Context::listDevices(bool verbose) {
    if(!isInited())
		init();

	stringstream stream;

	if(numTotal() == 0) {
		stream << "no devices found";
		return;
	}
	else if(numTotal() == 1) {
		stream << 1 << " device found";
	}
	else {
		stream << deviceList.size() << " devices found";
	}

	if(verbose) {
		ofLogVerbose("ofxRealSense2") << stream.str();
	}
	else {
		ofLogNotice("ofxRealSense2") << stream.str();
	}
	stream.str("");

	for(unsigned int i = 0; i < deviceList.size(); ++i) {
		stream << "    id: " << deviceList[i].id << " serial: " << deviceList[i].serial;
		if(verbose) {
			ofLogVerbose("ofxRealSense2") << stream.str();
		}
		else {
			ofLogNotice("ofxRealSense2") << stream.str();
		}
		stream.str("");
	}
}

int ofxRealSense2Context::numTotal() {
    if(!isInited())
		init();
	return rs2Context->query_devices().size();
}

int ofxRealSense2Context::numAvailable() {
	if(!isInited())
		init();
	return rs2Context->query_devices().size() - realsenses.size();
}

int ofxRealSense2Context::numConnected() {
	return realsenses.size();
}

int ofxRealSense2Context::getDeviceIndex(int id) {
	for(unsigned int i = 0; i < deviceList.size(); ++i) {
		if(deviceList[i].id == id)
			return i;
	}
	return -1;
}

/*
ofxRealSense2* ofxRealSense2Context::getKinect(freenect_device* dev) {
    std::map<int, ofxRealSense2*>::iterator iter;
    for(iter = kinects.begin(); iter != kinects.end(); ++iter) {
        if(iter->second->kinectDevice == dev){
            return iter->second;
        }
        return NULL;
    }
}
*/

int ofxRealSense2Context::getDeviceIndex(string serial) {
	for(unsigned int i = 0; i < deviceList.size(); ++i) {
		if(deviceList[i].serial == serial)
			return i;
	}
	return -1;
}

int ofxRealSense2Context::getDeviceId(int index) {
    if( index >= 0 && index < deviceList.size() ){
        return deviceList[index].id;
    }
	return -1;
}

int ofxRealSense2Context::getDeviceId(string serial){
	for(unsigned int i = 0; i < deviceList.size(); ++i) {
		if(deviceList[i].serial == serial){
			return deviceList[i].id;
        }
	}
	return -1;
}

bool ofxRealSense2Context::isConnected(int id) {
	std::map<int, ofxRealSense2*>::iterator iter = realsenses.find(id);
	return iter != realsenses.end();
}

bool ofxRealSense2Context::isConnected(string serial) {
	std::map<int, ofxRealSense2*>::iterator iter;
	for(iter = realsenses.begin(); iter != realsenses.end(); ++iter) {
		if(iter->second->getSerial() == serial)
			return true;
	}
	return false;
}

int ofxRealSense2Context::nextAvailableId() {
	if(!isInited())
		init();

	// a brute force free index finder :D
	std::map<int, ofxRealSense2*>::iterator iter;
	for(unsigned int i = 0; i < deviceList.size(); ++i) {
		iter = realsenses.find(deviceList[i].id);
		if(iter == realsenses.end())
			return deviceList[i].id;
	}
	return -1;
}

string ofxRealSense2Context::nextAvailableSerial() {
	if(!isInited())
		init();

	int id = nextAvailableId();
	if(id == -1) {
		return "";
	}
	return deviceList[getDeviceIndex(id)].serial;
}

