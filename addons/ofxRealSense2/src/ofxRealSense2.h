#pragma once

#include "ofMain.h"

//#include "libfreenect.h"
#include <librealsense2/rs.hpp>

#if defined(_MSC_VER) || defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)
    // do windows stuff
#else
    // mac and linux need this
    #include <libusb.h>
#endif



#include "ofxBase3DVideo.h"

class ofxRealSense2Context;

/// \class ofxRealSense
///
/// wrapper for a Real Sense v2 device
///
///
class ofxRealSense2 : public ofxBase3DVideo, protected ofThread {

public:

	ofxRealSense2();
	virtual ~ofxRealSense2();

/// \section Main

	/// initialize resources, must be called before open()
	/// infrared controls whether the video image is rgb or IR
	/// set video to false to disable video image grabbing (saves bandwidth)
	/// set texture to false if you don't need to use the internal textures
	///
	/// naturally, if you disable the video image the video pixels and
	/// RGB color will be 0
	bool init(bool infrared=true, bool video=true, bool texture=true);

	/// clear resources, do not call this while ofxRealSense is running!
	void clear();

	/// calibrates the depth image to align with the rgb image, disabled by default
	///
	/// call this before open(), has no effect while the connection is running
	///
	/// note: this calculation uses some cpu, leave off if not needed
	void setRegistration(bool bUseRegistration=false);

	/// open the connection and start grabbing images
	///
	/// set the deviceIndex to choose a RealSense, see numAvailableDevices()
	/// if you don't set the index (ie index=-1), the first available RealSense will be used
	///
	/// note: this has changed so that a deviceIndex of 0 will also open the same device, regardless of the order in which it was plugged in
	bool open(int deviceIndex=-1);

	/// open using a RealSense unique serial number
	///
	/// NOTE: currently, libfreenect returns a serial number with all 0s for
	bool open(string serial);

	/// close the connection and stop grabbing images
	void close();

	/// is the connection currently open?
	bool isConnected() const;
	bool isInitialized() const;

	/// is the current frame new?
	bool isFrameNew() const;
	bool isFrameNewVideo() const;
	bool isFrameNewDepth() const;

	bool setPixelFormat(ofPixelFormat pixelFormat);
	ofPixelFormat getPixelFormat() const;

	/// updates the pixel buffers and textures
	///
	/// make sure to call this to update to the latest incoming frames
	void update();

/// \section Depth Data

	/// get the calulated distance for a depth point
	float getDistanceAt(int x, int y) const;
	float getDistanceAt(const ofPoint & p) const;

	/// calculates the coordinate in the world for the depth point (perspective calculation)
	///
	/// center of image is (0.0)
	ofVec3f getWorldCoordinateAt(int cx, int cy) const;
	ofVec3f getWorldCoordinateAt(float cx, float cy, float wz) const;

/// \section Intrinsic IR Sensor Parameters

	/// these values are used when depth registration is enabled to align the
	/// depth image to the rgb image, see http://www.ros.org/wiki/kinect_calibration/technical
	///
	/// they could also be useful for real world accurate point clouds ... weee!

	/// get the distance between the IR sensor and IR emitter in cm
	float getSensorEmitterDistance() const;

	/// get the distance between the IR sensor and the RGB camera in cm
	float getSensorCameraDistance() const;

	/// get the size of a single pixel on the zero plane in mm
	float getZeroPlanePixelSize() const;

	/// get the focal length of the IR sensor in mm
	float getZeroPlaneDistance() const;

/// \section RGB Data

	/// get the RGB value for a depth point
	///
	/// see setRegistration() for calibrated depth->RGB points
	ofColor getColorAt(int x, int y) const;
	ofColor getColorAt(const ofPoint & p) const;

/// \section Pixel Data

	/// get the video pixels reference
	///
	/// see setRegistration() for a calibrated depth->RGB image
	ofPixels & getPixels();
	const ofPixels & getPixels() const;

	/// get the pixels of the most recent depth frame
	ofPixels & getDepthPixels();       	///< grayscale values
	const ofPixels & getDepthPixels() const;       	///< grayscale values
	ofShortPixels & getRawDepthPixels();	///< raw 11 bit values
	const ofShortPixels & getRawDepthPixels() const;	///< raw 11 bit values

	/// get the distance in millimeters to a given point as a float array
	ofFloatPixels & getDistancePixels();
	const ofFloatPixels & getDistancePixels() const;

	/// get the video (ir or rgb) texture
	ofTexture& getTexture();
	const ofTexture& getTexture() const;
	OF_DEPRECATED_MSG("Use getTexture() instead", ofTexture& getTextureReference());
	OF_DEPRECATED_MSG("Use getTexture() instead", const ofTexture& getTextureReference() const);

	/// get the grayscale depth texture
	ofTexture& getDepthTexture();
	const ofTexture& getDepthTexture() const;
	OF_DEPRECATED_MSG("Use getDepthTexture() instead", ofTexture& getDepthTextureReference());
	OF_DEPRECATED_MSG("Use getDepthTexture() instead", const ofTexture& getDepthTextureReference() const);

/// \section Grayscale Depth Value

	/// set the near value of the pixels in the grayscale depth image to white
	///
	/// bEnabled = true:  pixels closer to the camera are brighter (default)
	/// bEnabled = false: pixels closer to the camera are darker
	void enableDepthNearValueWhite(bool bEnabled=true);
	bool isDepthNearValueWhite() const;

	/// set the clipping planes for the depth calculations in millimeters
	///
	/// these are used for the depth value (12bit) -> grayscale (1 byte) conversion
	/// ie setting a short range will give you greater sensitivity from 0-255
	///
	/// default is 50cm - 4m
	/// note: you won't get any data < 50cm and distances > 4m start to get noisy
	void setDepthClipping(float nearClip=500, float farClip=4000);
	float getNearClipping() const;
	float getFarClipping() const;

/// \section Query Capabilities

	/// check for device capabilites ...
	/// motor, led, or accelerometer control isn't currently supported
	/// by libfreenect with newer Kinect models (> 1414)
	bool hasAccelControl() const;
	bool hasCamTiltControl() const;
	bool hasLedControl() const;

/// \section Accelerometer Data

	/// get the XYZ accelerometer values
	///
	/// ... yes, the kinect has an accelerometer

	/// raw axis values
	ofPoint getRawAccel() const;

	/// axis-based gravity adjusted accelerometer values
	///
	/// from libfreeenect:
	///
	/// as laid out via the accelerometer data sheet, which is available at
	///
	/// http://www.kionix.com/Product%20Sheets/KXSD9%20Product%20Brief.pdf
	///
	ofPoint getMksAccel() const;

    /// get the current pitch (x axis) & roll (z axis) of the kinect in degrees
    ///
    /// useful to correct the 3d scene based on the camera inclination
    ///
	float getAccelPitch() const;
	float getAccelRoll() const;

/// \section Camera Tilt Motor

	/// set tilt angle of the camera in degrees
	/// 0 is flat, the range is -30 to 30
	bool setCameraTiltAngle(float angleInDegrees);

	/// get the current angle
	float getCurrentCameraTiltAngle() const;

	/// get the target angle (if the camera is currently moving)
	float getTargetCameraTiltAngle() const;

/// \section LED

	enum LedMode {
		LED_DEFAULT = -1, // yellow when not running, green when running
		LED_OFF = 0,
		LED_GREEN = 1,
		LED_RED = 2,
		LED_YELLOW = 3,
		LED_BLINK_GREEN = 4,
		LED_BLINK_YELLOW_RED = 6
	};

    /// set the current led color and/or blink mode,
	/// only applied while the kinect is open
    //void setLed(ofxKinect::LedMode mode);

/// \section Draw

	/// enable/disable frame loading into textures on update()
	void setUseTexture(bool bUse);
	bool isUsingTexture() const;

	/// draw the video texture
	void draw(float x, float y, float w, float h) const;
	void draw(float x, float y) const;
	void draw(const ofPoint& point) const;
	void draw(const ofRectangle& rect) const;

	/// draw the grayscale depth texture
	void drawDepth(float x, float y, float w, float h) const;
	void drawDepth(float x, float y) const;
	void drawDepth(const ofPoint& point) const;
	void drawDepth(const ofRectangle& rect) const;

/// \section Util

	/// get the device id
	/// returns -1 if not connected
	int getDeviceId() const;

	/// get the unique serial number
	/// returns an empty string "" if not connected
	///
	/// NOTE: currently, libfreenect returns a serial number with all 0s for
	/// kinect models > 1414, so this will only work with the original xbox kinect
	string getSerial() const;

	/// static RealSense image size
	const static int width = 640;
	const static int height = 480;
	float getHeight() const;
	float getWidth() const;

	bool is_d435 = false;

/// \section Static global RealSense context functions

	/// print the device list
	static void listDevices();

	/// get the total number of devices
	static int numTotalDevices();

	/// get the number of available devices (not connected)
	static int numAvailableDevices();

	/// get the number of currently connected devices
	static int numConnectedDevices();

	/// is a device already connected?
	static bool isDeviceConnected(int id);
	static bool isDeviceConnected(string serial);

	/// get the id of the next available device,
	/// returns -1 if nothing found
	static int nextAvailableId();

	/// get the serial number of the next available device,
	/// returns an empty string "" if nothing found
	static string nextAvailableSerial();

protected:

	int deviceId;	///< -1 when not connected
	string serial;	///< unique serial number, "" when not connected

	bool bUseTexture;
	ofTexture depthTex; ///< the depth texture
	ofTexture videoTex; ///< the RGB texture
	bool bGrabberInited;

	ofPixels videoPixels;
	ofPixels depthPixels;
	ofShortPixels depthPixelsRaw;
	ofFloatPixels distancePixels;

	ofPoint rawAccel;
	ofPoint mksAccel;

	float targetTiltAngleDeg;
	float currentTiltAngleDeg;
	bool bTiltNeedsApplying;

    int currentLed;
    bool bLedNeedsApplying;
    bool bHasMotorControl; // cam tilt motor

	// for auto connect tries
	float timeSinceOpen;
	int lastDeviceId;
	bool bGotData;
	int tryCount;

private:

    int DEPTH_X_RES = 640;
    int DEPTH_Y_RES = 480;

	friend class ofxRealSense2Context;

	/// global statics shared between RealSense instances
	static ofxRealSense2Context realSense2Context;

	rs2::pipeline pipeline;
	rs2::device realSense2Device;
	rs2_intrinsics rgb_intr;
	rs2_intrinsics depthr_intr;


	ofShortPixels depthPixelsRawIntra;	///< depth back
	ofPixels videoPixelsIntra;			///< rgb back
	ofShortPixels depthPixelsRawBack;	///< depth back
	ofPixels videoPixelsBack;			///< rgb back

	vector<unsigned char> depthLookupTable;
	void updateDepthLookupTable();
	void updateDepthPixels();

	bool bIsFrameNewVideo, bIsFrameNewDepth;
	bool bNeedsUpdateVideo, bNeedsUpdateDepth;
	bool bGrabVideo;
	bool bUseRegistration;
	bool bNearWhite;

	float nearClipping, farClipping;

	bool bIsVideoInfrared;  ///< is the video image infrared or RGB?
	int videoBytesPerPixel; ///< how many bytes per pixel in the video image
	ofPixelFormat pixelFormat;

	/// thread function
	void threadedFunction();
};

/// \class ofxRealSense2Context
///
/// wrapper for the freenect context
///
/// do not use this directly
///
class ofxRealSense2Context {

public:

	ofxRealSense2Context();
	~ofxRealSense2Context();

/// \section Main

	/// init the freenect context
	bool init();

	/// clear the freenect context
	/// closes all currently connected devices
	void clear();

	/// is the context inited?
	bool isInited();

	/// open a RealSense device
	/// an id of -1 will open the first available
	bool open(ofxRealSense2& realSense, int id=-1);

	/// open a RealSense device by it's unique serial number
	bool open(ofxRealSense2& realSense, string serial);

	/// close a RealSense device
	void close(ofxRealSense2& realSense);

	/// closes all currently connected RealSenses
	void closeAll();

/// \section Util

	/// (re)build the list of devices
	void buildDeviceList();

	/// print the device list
	void listDevices(bool verbose=false);

	/// get the total number of devices
	int numTotal();

	/// get the number of available devices (not connected)
	int numAvailable();

	/// get the number of currently connected devices
	int numConnected();

	/// get the RealSense object from a device pointer
	/// returns NULL if not found
	ofxRealSense2* getRealSense(rs2::device* dev);

	/// get the deviceList index from an id
	/// returns -1 if not found
	int getDeviceIndex(int id);

	/// get the deviceList index from an id
	/// returns -1 if not found
	int getDeviceIndex(string serial);

	/// get the deviceList id from an index
	/// returns -1 if not found
    int getDeviceId(int index);

	/// get the deviceList id from a serial
	/// returns -1 if not found
    int getDeviceId(string serial);

	/// is a device with this id already connected?
	bool isConnected(int id);

	/// is a device with this serial already connected?
	bool isConnected(string serial);

	/// get the id of the next available device,
	/// returns -1 if nothing found
	int nextAvailableId();

	/// get the serial number of the next available device,
	/// returns an empty string "" if nothing found
	string nextAvailableSerial();

	/// get the raw pointer
	rs2::context* getContext() {return rs2Context;}

	// for auto-enumeration
    struct RealSensePair{
		string serial;	///< unique serial number
		int id;			///< freenect bus id
    };

private:

	bool bInited;						///< has the context been initialized?
	rs2::context* rs2Context;    ///< RealSense context handle
	std::vector<RealSensePair> deviceList;	///< list of available devices, sorted by serial lexicographically
	std::map<int, ofxRealSense2*> realsenses;   ///< the connected RealSenses
};
