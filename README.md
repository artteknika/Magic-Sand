# Magic Sand for RealSense
Magic Sand for RealSense is a software for operating an augmented reality sandbox like the [Augmented Reality Sandbox](https://arsandbox.ucdavis.edu)
developped by [UC Davis](http://idav.ucdavis.edu/~okreylos/ResDev/SARndbox/).

This project is based on [Magic-Sand](https://github.com/thomwolf/Magic-Sand), and instead of Kinect, [Intel RealSense D435](https://www.intelrealsense.com/depth-camera-d435/) is made to operate as a sensing device.

It was developed with specific aim of simplifying the use of an augmented reality sandbox for RealSense in a home/family environment:

- run on a mid-range laptop or home computer (macOS 10.14.6 Mojave operation has been confirmed, minimal GPU requirement).
- openFrameworks 0.9.8
- Xcode version 9.4.1
- [RealSense SDK](https://github.com/IntelRealSense/librealsense) version 2.27.0
- RealSense
  - Intel RealSense D435 (Firmware 5.11.11.100)

## Main Features

Operates on a computer connected to a home cinema projector and a RealSense sensor.
The software controls the projector to project colors as a function of the sand level measured by the RealSense sensor and transforms a sandbox in a colorful playground.

## Getting started

The easiest way to get started is to build the physical setup provided in the guide found at the [tutorial page](https://imgur.com/gallery/Q86wR) and/or check the [reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/)

Secondly, download and install/unpack the latest ready-to-use version of the software at the [release page](https://github.com/thomwolf/Magic-Sand/releases/latest). Follow the instructions on the release page to download and install the necessary drivers.

### Setting up the system

Connect and turn on the projector and the RealSense and start the software.

By default the software starts in a **setup** mode where the depth or color image from the RealSense can be seen in the user interface and the projector projects a completely white image. This way it is easy to check if the RealSense is running and if the projector is working. The status of the RealSense and the projector can be seen in the status window to the lower left in the user interface.

In **setup** mode the physical positions of the RealSense and projector can be optimised. 

### Calibration

To calibrate the system so the RealSense and the projector is in correspondence a few steps are needed:
- Flatten the sand in the sand box.
- Make sure that you see either the depth image or the color image from the RealSense (click **advanced|Display Rs2 Depth View**)
- Press **Calibration|Manually Draw Sand Region** 
- Define the sand region by drawing a rectangle with the mouse on the RealSense Depth or Color view
- Press **Automatically Calibrate Rs2 & Projector** - a series of Chessboard patterns are now projected  on the sand.
- When a prompt appears cover the sand box with a light piece of cardboard or similar
- Press ok -  a series of Chessboard patterns are now projected on the cardboard/plate.

If the calibration is succesful the status window should be updated showing that all is ok.

This application was modified Status GUI, it shows Application Status, RealSense Status, Calibration ROI and, the status of Calibration.
RealSense requires buffer time for calibration, so we added The Status GUI shows ready for calibration.
Please check it before calibration, if you calibrate before ready for calibration is No, pop up a screen for saving a image.

#### Debug mode for calibration

If the calibration was not succesful a debug mode can be enabled that will place debug files in the **data\DebugFiles** folder. These might point you in the direction of why the calibration failed. Do this by enabling **advanced|Dump Debug** and run the calibration routine again.

## Starting the Application

If the calibration was succesful or if a calibration was done before, the application can be started by pressing space or pushing the **Run** button.

Now a colored map with iso-lines should appear on the sand. The framerate should be close to 60 FPS for modern PCs.

### Caution

If you finish this application, RealSense is sometimes left to be locked in this application or RealSense Viewer. Then Disconnect RealSense.

## Sandbox games

There are a few games included in Magic-Sand.
But, we disabled this Sandbox games. If you want to enable it, edit source code of this software.

### Shape an Island

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### The Sandimals 2-player game

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

### The animal and their mothers game

Refer to [Magic-sand](https://github.com/thomwolf/Magic-Sand).

## Coding and Extending Magic Sand

### Source Code

This software source code based on [Magic-sand](https://github.com/thomwolf/Magic-Sand). But you are only able to launch this software by connecting RealSense. This software needs to connect RealSense.

### Create development environment

#### Installation packages, SDK

We need development environment when add functions to this software.
Please launch this command

```
# Clone IntelRealSense/librealsense
git clone https://github.com/IntelRealSense/librealsense.git

# Get needs package
brew install libusb pkg-config
brew install glfw
brew install cmake

# Setup build
cd ./librealsense # move library
git reset --hard 719b0b9 # Do downgrade version 2.13.0
mkdir build && cd build # make build dir and move build
sudo xcode-select --reset # init xcode-select
cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode # cmake librealsense

# Open
open librealsense2.xcodeproj
```

#### After opening XCode
  - if it build successful, build target change `install`, launch SDK.
  - After build successful, Check `/usr/local/lib/librealsense2.dylib`

#### SDK path for Magic-Sand
  - open cloned magic-sand project by Xcode.
  - After opening magic-sand, select project build setting.
  - Add RealSense dylib path `/usr/local/lib/librealsense2.dylib` to `Other Linker Flags` path list
  - Add RealSense header path `/usr/local/include` to `Header Search Paths` path list

### Dependencies
Magic Sand for RealSense2 is based on [openframeworks](http://openframeworks.cc/) release 0.9.8 and makes use of the following addons:
- official addons (included in openframeworks 0.9.8)
  * ofxOpenCv
  * ofxKinect (using USB lib)
  * ofxXmlSettings
- community addons:
  * [ofxCv](https://github.com/kylemcdonald/ofxCv)
  * [ofxParagraph](https://github.com/braitsch/ofxParagraph)
  * [ofxDatGui (forked version)](https://github.com/thomwolf/ofxDatGui)
  * [ofxModal](https://github.com/braitsch/ofxModal)
- local addons:
  * ofxRealSense2 (included in this project)

### Quick start for editing the source code
- Grab a copy of [openframeworks](http://openframeworks.cc/download/) for your OS. (This software needs openFrameworks version 0.9.8.)
- Unpack the Magic-Sand in the **app/myApps** folder in the openframeworks directory tree
- Grab the additionnal community addons listed above. They should be installed in the **addons** subdirectory of openframeworks
- If you are a windows user, install the kinect drivers as detailed on the [release page](https://github.com/thomwolf/Magic-Sand/releases/latest)
- Enjoy ! (Xcode / VS2015 project files are supplied, should work also on linux)

On Xcode 10 or later, **libs/openFrameworksCompiled/project/osx/CoreOF.xcconfig** file in the openFrameworks needs to be modified.
Delete `i386` from` VLID_ARCHS` and delete `-framework QuickTime` from` OF_CORE_FRAMEWORKS` in the **CoreOF.xcconfig** file.

Be sure to check the [openframeworks](http://openframeworks.cc/) documentation and forum if you don't know it yet, it is an amazing community !

### How it can be used
The code was designed trying to be easily extendable so that additional games/apps can be developed on its basis.

Note that some of the below descriptions are slightly out-of-date.

The `Rs2Projector` class handles the communication with the RealSense sensor, the calibration and the coordinates conversions between RealSense (2D), world (3D) and projector (2D) coordinate systems.

You can create a `Rs2Projector` object as a `shared_ptr` in the `setup()` function of your openframeworks app. It requires a pointer to the projector window (see provided `main.cpp` on how to properly setup two windows in openframeworks and get a pointer to the projector window).

The `Rs2Projector` object can be shared among the various objects that need access to depth and conversions functions (not multi-thread proof of course).

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

### Rs2Projector Functions

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
- the RealSense coordinate system of the 2D realsense image : (x, y) in pixel units with origin in the top-left corner,
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

#### Setup & calibration functions
`startFullCalibration()` perfoms an automatic calibration of the realsense and the projector.
An automatic calibration comprises:
- ask the user to flatten the sand,
- measure the average plane formed by the sand surface to define the base plane (see above),
- display and find 5 chess boards (60 calibration points) on the sand surface,
- ask the user to cover the sand with a board,
- display and find 10 chess boards (60 calibration points) on the board surface,
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
- `isROIUpdated()`: was the sand region location/extension updated in the previous call to `update()`
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
## [1.5.4.1 for RealSense](https://github.com/artteknika/Magic-Sand-for-RealSense/releases/tag/v1.5.4.1-for-RealSense) - 05-09-2019
Supported RealSense SDK 2

## Changed
- refer to Point to change
- We can run this software of RealSense

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
