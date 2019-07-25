# Magic Sand For RealSense2
Magic Sand is a software for operating an augmented reality sandbox like the [Augmented Reality Sandbox](https://arsandbox.ucdavis.edu)
developped by [UC Davis](http://idav.ucdavis.edu/~okreylos/ResDev/SARndbox/).

It was developed with specific aim of simplifying the use of an augmented reality sandbox for RealSense2 in a home/family environment:

- run on a mid-range laptop or home computer (Mac OS X High Sierra ver 10.13.6 operation has been confirmed, minimal GPU requirement).
- openFrameworks of_v0.9.8_osx_release
- [RealSense SDK](https://github.com/IntelRealSense/librealsense) ver v2.13.0
- RealSense2 version
  * Intel Real Sense2 435

## Main Features

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Getting started

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Setting up the system

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Calibration

Basically refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).
This application was modified Status GUI, it shows Application Status, RealSense2 Status, Calibration ROI and, the status of Calibration.
RealSense2 requires buffer time for calibration, so this was added The Status GUI shows ready for calibration.
Please check it before calibration.

#### Debug mode for calibration

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Starting the Application

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Caution

If you finish this application, RealSense2 is sometimes left to be locked in this application or RealSense Viewer. Then Disconnect RealSense2.

## Sandbox games

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).
But, we disabled this Sandbox games. If you want to enable it, edit source code of this software.

### Shape an Island

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### The Sandimals 2-player game

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### The animal and their mothers game

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Coding and Extending Magic Sand

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### Source Code

This software source code based on [Magic-sand](https://github.com/thomwolf/Magic-Sand). But you are only able to launch this software by connecting realsense2. This software needs to connect RealSense2.

### Create development environment

#### Installation packages, SDK

We need development environment when add functions to this software.
Please launch this command

```
cd ~/oF/of_v0.9.8_osx_release/apps/ # change openFrameworks dir
git clone this software link # clone this software

git clone https://github.com/IntelRealSense/librealsense.git # clone required RealSense SDK

brew install ghq # installation ghq package

# get required package
brew install libusb pkg-config
brew install glfw
brew install cmake

# setup Build
cd ~/.ghq/github.com/IntelRealSense/librealsense # move library
git reset --hard 719b0b9 # Do downgrade version 2.13.0
mkdir build && cd build # make build dir and move build
sudo xcode-select --reset # init xcode-select
cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode # cmake librealsense

# Open
open librealsense2.xcodeproj
```

#### After opening XCode

  - Build target change `All` to `realsense-viewer`, launch SDK.

  - if it build successful, build target change `import`, launch SDK.

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

### Point to change

    Note Changed scripts

      * `KinectProjector` -> `Rs2Projector`
      * `KinectGrabber` -> `Rs2Grabber`
      * `KinectProjectorCalibration` -> `Rs2ProjectorCalibration`
      * `SandSurfaceRenderer`

### How it can be used


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

The `rs2Projector` object needs full control on the projector window during the calibration process, so you should be careful not to draw things on the projector window after the call to `rs2Projector->drawProjectorWindow()` if a calibration is currently performed (you can check `rs2Projector->isCalibrating()`).

The following example illustrates the `update()` and `draw()` functions to implement a simple augmented reality sandbox once the `rs2Projector` and `sandSurfaceRenderer` objects have been initiated as detailed above and provided that the projector window has a listener callback setup to the `drawProjWindow(ofEventArgs &args)` function (see `main.cpp`).

```
void ofApp::update(){
  rs2Projector->update();
  sandSurfaceRenderer->update();
}
void ofApp::drawProjWindow(ofEventArgs &args){
  rs2Projector->drawProjectorWindow();

  if (!rs2Projector->isCalibrating()){
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
float elevationAtRs2Coord(float x, float y);
float elevationToRs2Depth(float elevation, float x, float y);
```

`Rs2Projector` also store a matrix of gradients of the realsense depth in the world coordinate system (slope of the sand) computed with a given resolution (with a 10 pixels bin by default).
The gradient at a given location can be accessed by:
```
ofVec2f gradientAtRs2Coord(float x, float y);
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

#### RealSense2 projector state functions

The following functions give information of the state of the realsense object:
- `isCalibrating()`: is the `rs2Projector` currently performing a calibration
- `isCalibrated()`: is the `rs2Projector` calibrated (calibration file found or calibration performed)
- `isImageStabilized()`: is the depth frame stabilized (arbitrary time frame after initialisation)
- `isBasePlaneUpdated()`: was the base plane updated in the previous call to `update()'
- `isROIUpdated()`: was the sand region location/extension updated in the previous call to `update()'
- `isCalibrationUpdated()`: was the calibration updated in the previous call to `update()`

#### RealSense2 projector other getters
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

## points to change

### Script, Directory name
    * `KinectProjector` -> `Rs2Projector`
        - `KinectGrabber.cpp` -> `Rs2Grabber.cpp`
        - `KinectGrabber.h` -> `Rs2Grabber.h`
        - `KinectProjector.h` -> `Rs2Projector.h`
        - `KinectProjector.cpp` -> `Rs2Projector.cpp`
        - `KinectProjectorCalibration.h` -> `Rs2ProjectorCalibration.h`
        - `KinectProjectorCalibration.cpp` -> `Rs2ProjectorCalibration.cpp`
    * `SandSurfaceRenderer`
        - `SandSurfaceRenderer.cpp`
        - `SandSurfaceRenderer.h`
        - `ColorMap.h`
        - `ColorMap.cpp`
#### main points of modification
    * `Rs2Projector`
        - `Rs2Grabber.cpp`
        - `Rs2Grabber.h`
        - `Rs2Projector.h`
        - `Rs2Projector.cpp`
    * `SandSurfaceRenderer`
        - `SandSurfaceRenderer.cpp`

#### minor changes
    * `Rs2Projector`
        - `Rs2ProjectorCalibration.h`
        - `Rs2ProjectorCalibration.cpp`
    * `SandSurfaceRenderer`
        - `SandSurfaceRenderer.h` 
        - `ColorMap.h`
        - `ColorMap.cpp`
# Changelog

## [1.5.4.1 for RealSense2]() - - -2019
Supported RealSense2
### Bug fixes
    - a draw depth frame was broken in 1.5.4.1 - it did not show depth frame. Now fixed

## Changed
    - refer to Point to change
    - We can run this software of RealSense2

## [1.5.4.1](https://github.com/thomwolf/Magic-Sand/releases/tag/v1.5.4.1) - 10-10-2017
Bug fix release

### Bug fixes
    - The calibration procedure was broken in 1.5.4 - it did not show the checkerboard. Now fixed.

### Added
    - Linux make files (experimental)

## [1.5.4](https://github.com/thomwolf/Magic-Sand/releases/tag/v1.5.4) - 23-09-2017

Minor release of Magic-Sand-with-Games

### Added
    - Kinect FPS counter for received frames
    - XCode build files
    - Full frame filter option
    - Simple InPainting option for removing outliers in the depth map
    - Better scaling of GUI
    - Debug feature: Kinect ROI can be seen on projector
    - Debug feature: Left mouse click in Kinect depth view will print depth coordinates on console
    - ChangeLog to the README

### Changed
    - Animals in animal game is now flipped depending on Kinect-projector matrix - so hopefully no more backwards swimming
    - GUI update for animal game. Now updates animal numbers
    - Adjusted game constants for animal game.
    - Added beginner/novice/normal/expert game mode. Press 1, 2, 3 or 4 to start the different modes.

### Bug fixes
    - Spatial filter now correctly filters the ROI

## [1.5.0](https://github.com/thomwolf/Magic-Sand/tree/v1.5) - 08-08-2017
        Initial release of Magic-Sand with Games
