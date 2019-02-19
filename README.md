# Magic Sand For RealSense2
This software base on [Magic-sand](https://github.com/thomwolf/Magic-Sand).

It was developed woth specific aim of simplifying the use of an augmented reality sandbox for RealSense2 in a home/family environment:

- run on a mid-range laptop or home computer (Mac OS X High Sierra ver 10.13.6 operation has been confirmed, minimal GPU requirement).
- openFrameworks of_v0.9.88_osx_release
- [RealSense SDK](https://github.com/IntelRealSense/librealsense) ver v2.13.0
- RealSense2 version
  * Intel Real Sense2 435 ()

## Main Features

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Getting started

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Setting up the system

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Calibration

Basically refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).
I changed Application Status GUI, it shows Application Status, RealSense2 Status, Calibration ROI, the status of Calibration.
RealSense2 requires time buffer for calibration, so I added The Status GUI shows ready for calibration.
Please check it before calibration.

#### Debug mode for calibration

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Starting the Application

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Sandbox games

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).
But, I disabled this Sandbox games. If you want to unable it, edit source code of this software.

### Shape an Island

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### The Sandimals 2-player game

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### The animal and their mothers game

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Coding and Extending Magic Sand

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Source Cod

This software source code based on [Magic-sand](https://github.com/thomwolf/Magic-Sand). But you aren't able to launch this software by connecting kinect. This software needs to connect RealSense2.

### Create development environment

#### installation packages, SDK

We need development environment when add functions to this software.
Please launch this command
```
cd ~/oF/of_v0.9.8_osx_release/apps/ # change openFrameworks dir
git clone this software link # clone this software

brew install ghq # installation ghq package
ghq get ghq get IntelRealSense/librealsense # clone RealSense2 SDK to ~/.ghq

# get require package
brew install libusb pkg-config
brew install glfw
brew install cmake

# setup Build
cd ~/.ghq/github.com/IntelRealSense/librealsense # move library
git reset --hard 719b0b9 # change version 2.13.0
mkdir build && cd build # make build dir and move build
sudo xcode-select --reset # init xcode-select
cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode # cmake librealsense

# Open
open librealsense2.xcodeproj
```
#### After opening XCode

  - Build target change `All` to `realsense-viewer`, launch SDK.

  - if build successful, build target change `import`, launch SDK.

  - After build successful, Check `/usr/local/lib/librealsense2.2.dylib`

#### SDK path for Magic-Sand

  - open cloned magic-sand project by Xcode.

  - After opening magic-sand, select project build setting.

  - Search `OTHER LINKER FLAGS`

  - Add RealSense2 dylib path `/usr/local/lib/librealsense2.2.dylib` to `OTHER LINKER FLAGS` path list

### Dependencies
Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).
this software needs ofxRealSense2.
- official addons (included in openframeworks 0.9.8)
  * ofxOpenCv
  * ofxKinect (using USB lib)
  * ofxXmlSettings
- community addons:
  * [ofxCv](https://github.com/kylemcdonald/ofxCv)
  * [ofxParagraph](https://github.com/braitsch/ofxParagraph)
  * [ofxDatGui (forked version)](https://github.com/thomwolf/ofxDatGui)
  * [ofxModal](https://github.com/braitsch/ofxModal)
  * [ofxRealSense2]()

### Quick start for editing the source code
Basically Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).
This software needs openFrameworks version 0.9.8.

### How it can be used

Note Changed scripts

  * `KinectProjector` -> `Rs2Projector`
  * `KinectGrabber` -> `Rs2Grabber`
  * `KinectProjectorCalibration` -> `Rs2ProjectorCalibration`
  * `SandSurfaceRenderer`

`Rs2Projector` class handles the communication with the realsense2 sensor, the calibration and the coordinates conversions between realsense2 (2D), world (3D) and projector (2D) coordinate systems.

`Rs2Projector` object as a `shared_ptr` in the `setup()` function of your openframeworks app. It requires a pointer to the projector window (see provided `main.cpp` on how to properly setup two windows in openframeworks and get a pointer to the projector window).

`Rs2Projector` object can be shared among the various objects that need access to depth and conversions functions (not multi-thread proof of course).

For instance, a `SandSurfaceRenderer` object can be constructed with a pointer to the `Rs2Projector` shared object. (the `SandSurfaceRenderer` class convert depth information in color using a editable colormap and display these colors on the sand).

A typical `setup()` function of a openframeworks app can thus reads:
```
std::shared_ptr<ofAppBaseWindow> projWindow;
std::shared_ptr<Rs2Projector> rs2Projector;
SandSurfaceRenderer* sandSurfaceRenderer;

void ofApp::setup() {
	rs2Projector = std::make_shared<Rs2Projector>(projWindow);
	rs2Projector->setup(true);

	sandSurfaceRenderer = new SandSurfaceRenderer(rs2Projector, projWindow);
	sandSurfaceRenderer->setup(true);
}
```
`setup(true)` indicates that the GUI of the `Rs2Projector` and the `sandSurfaceRenderer` will be displayed.


The `rs2Projector` object then needs to be updated in the `update()` function of the openframeworks app (preferably before the objects that use its functions) and drawn in the projector `draw()` function.

The `rs2Projector` object needs full control on the projector window during the calibration process so you should be careful not to draw things on the projector window after the call to `rs2Projector->drawProjectorWindow()` if a calibration is currently performed (you can check `rs2Projector->isCalibrating()`).

The following example illustrates the `update()` and `draw()` functions to implement a simple augmented reality sandbox once the `rs2Projector` and `sandSurfaceRenderer` objects have been initiated as detailed above and provided that the projector window has a listener callback setup to the `drawProjWindow(ofEventArgs &args)` function (see `main.cpp`).

```
void ofApp::update(){
  rs2Projector->update();
  sandSurfaceRenderer->update();
}
void ofApp::drawProjWindow(ofEventArgs &args){
  rs2Projector->drawProjectorWindow();

  if (!kinectProjector->isCalibrating()){
      sandSurfaceRenderer->drawProjectorWindow();
      fboVehicles.draw(0,0);
  }
}
```

Source: [Magic-sand](https://github.com/thomwolf/Magic-Sand)

### realsense2Projector Functions

#### Shader functions
The `sandSurfaceRenderer` class shows example of shaders that can be used to compute color and how to set uniforms.

The following function of `Rs2Projector` are of special interest to setup a uniform.

```
void bind();
void unbind();
ofMatrix4x4 getTransposedRs2WorldMatrix();
ofMatrix4x4 getTransposedRs2ProjMatrix();
```
The `sampler2DRect` received in the shader is normalized between 0 and 1, a conversion scale thus has to be also sent.

#### Coordinate conversion / elevation functions
Three coordinate systems can be used:
- the realsense2 coordinate system of the 2D realsense image : (x, y) in pixel units with origin in the top-left corner,
- the world coordinate system: a 3D coordinate system (x, y, z) in millimeters units originating from the realsense sensor with z axis extending from the realsense sensor, x the horizontal axis of the realsense sensor and y the vertical axis, and
- the projector coordinate system of the 2D projector image : (x, y) in pixel units with origin in the top-left corner.

The most straighforward conversion goes from realsense coordinates to world coordinate system and projector coordinate system.
If you want to animate or display objects, a natural choice would thus be to store then in realsense coordinate and to perform the conversion on display.

The following functions provide conversions between the coordinate systems:
```
ofVec2f worldCoordToProjCoord(ofVec3f vin);
ofVec3f projCoordAndWorldZToWorldCoord(float projX, float projY, float worldZ);
ofVec2f rs2CoordToProjCoord(float x, float y);
ofVec3f rs2CoordToWorldCoord(float x, float y);
ofVec2f worldCoordTors2Coord(ofVec3f wc);
```

Another value that can be used is the `elevation` which is the distance from a point in world coordinate to a 3D base plane of that is defined by:
- a normal (`getBasePlaneNormal()`) and an offset (`getBasePlaneOffset()`), or
- a plane equation (`getBasePlaneEq()`).

`elevation` can be converted/accessed by the following functions:
```
float elevationAtKinectCoord(float x, float y);
float elevationToKinectDepth(float elevation, float x, float y);
```

`Rs2Projector` also store a matrix of gradients of the realsense depth in the world coordinate system (slope of the sand) computed with a given resolution (with a 10 pixels bin by default).
The gradient at a given location can be accessed by:
```
ofVec2f gradientAtKinectCoord(float x, float y);
```

Source: [Magic-sand](https://github.com/thomwolf/Magic-Sand)

#### Setup & calibration functions
`startFullCalibration()` perfoms an automatic calibration of the realsense and the projector.
An automatic calibration comprises:
- ask the user to flatten the sand,
- measure the average plane formed by the sand surface to define the base plane (see above),
- display and find 5 chess boards (60 calibration points) on the sand surface,
- ask the user to cover the sand with a board,
- display and find 5 chess boards (60 calibration points) on the board surface,
- set the detection ceiling to 50 milimeters above the board.

The following functions can be called to change some internal values of `rs2Projector`:
- `setGradFieldResolution(int gradFieldResolution)`: change the resolution of the gradient field
- `setSpatialFiltering(bool sspatialFiltering)`: toggle the spatial filtering of the depth frame
- `setFollowBigChanges(bool sfollowBigChanges)`: toggle "big change" detection (follow the hand of the user).

#### Kinect projector state functions

The following functions give information of the state of the realsense object:
- `isCalibrating()`: is the `rs2Projector` currently performing a calibration
- `isCalibrated()`: is the `rs2Projector` calibrated (calibration file found or calibration performed)
- `isImageStabilized()`: is the depth frame stabilized (arbitrary time frame after initialisation)
- `isBasePlaneUpdated()`: was the base plane updated in the previous call to `update()'
- `isROIUpdated()`: was the sand region location/extension updated in the previous call to `update()'
- `isCalibrationUpdated()`: was the calibration updated in the previous call to `update()`

#### Kinect projector other getters
The following functions give additional information :
- `getRs2ROI()`: get the sand region location/extension
- `getRs2Res()`: get the realsense resolution
- `getBasePlaneNormal()` : see above
- `getBasePlaneOffset()` : see above
- `getBasePlaneEq()` : see above

## Main differences with [SARndbox](https://github.com/KeckCAVES/SARndbox)

Magic Sand is a cross-platform project while SARndbox currently is only Linux. SARndbox is inherited from a larger VR toolbox that makes is somewhat daunting to start modifying. We hope that Magic Sand is slightly easier to start with.

Magic Sand uses the build-in registration feature of the realsense to perform an automatic calibration between the projector and the realsense sensor and does not use a pixel based depth calibration.

It is thus probably less acurate than SARndbox.

Magic Sand does not provide dynamic rain features (typically require a stronger GPU than the graphic card provided on a laptop).

Source: [Magic-sand](https://github.com/thomwolf/Magic-Sand)

# Changelog
Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand)
