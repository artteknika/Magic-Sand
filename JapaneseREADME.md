# Magic Sand For RealSense2
Magic Sand For RealSense は [Augmented Reality Sandbox](https://arsandbox.ucdavis.edu) のような砂場をマッピングするソフトウェアです．
開発元 [UC Davis](http://idav.ucdavis.edu/~okreylos/ResDev/SARndbox/).

このプロジェクトは [Magic-Sand](https://github.com/thomwolf/Magic-Sand) をベースに作られています．

この開発環境と実行環境は以下のようになっています．
 - 実行環境
  * 開発者向けノート，デスクトップ(Max OS X High Sierra Ver.10.13.6 GPU必須 実行確認済み環境)
  * openFrameworks of_v0.9.88_osx_release
 - 開発環境
  * Max OS X High Sierra Ver.10.13.6
  * openFrameworks of_v0.9.8_osx_release
  * [RealSense SDK](https://github.com/IntelRealSense/librealsense) ver v2.13.0
 - ハードウェア
  * Intel Real Sense 2 435 (動作確認済み)
## Main Features

プロジェクターとRealSense2をPCに繋げます．
プロジェクターのセンシングを行うために，キャリブレーションを行います．
キャリブレーション成功後，RealSense2から深さのセンシングを行い，そのセンシングのデータを元にプロジェクターに投影します．

## Getting started

砂場作成や設定について

[tutorial page](https://imgur.com/gallery/Q86wR) に設定のことについて記述されています．　
[reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/)　も参考にしてください．

また，[tutorial page](https://imgur.com/gallery/Q86wR)と[reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/)はハードウェアがKinectで行なっていますが，このソフトウェアはRealSense2で行います．

### Setting up the system

プロジェクターとRealSense2をPCに繋げて，magic-sandを実行してください

magic-sandを実行すると，深さとカラーイメージをRealSense2が取得していて，RealSense2からプロジェクターから投影されている白のイメージが見える事を確認してください．もしStatusが表示されない場合は，メインウィンドウのサイズを変更すると表示されます．
ステータスの中のパラメータは以下のようになっています．
  - アプリケーションの状態
  - RealSense2が認識できているかの状態
  - キャリブレーション時のROIが定義されているかの状態
  - ベースプレーンが認識できているかの状態
  - RealSense2が較正できているかの状態
  - キャリブレーションのステップ
  - キャリブレーション時のエラー回数
  - プロジェクターの解像度

In **setup** mode the physical positions of the Kinect and projector can be optimised.

### Calibration

キャリブレーションを行う際にRealSense2とProjectorに必要な設定があります．
- 投影する砂箱を平坦にしてください
- RealSense2 から深さのイメージとカラーイメージがとれているかを確認してください． **advanced|Display Rs2 Depth View** で確認
- **Calibration|Manually Draw Sand Region** を押してください．
- カラーイメージもしくは深さイメージのどちらかでキャリブレーションの範囲を四角形を定義してください．
- **Automatically Calibrate Kinect & Projector** をおして一連のキャリブレーションの流れを行なってください
- `Please cover the sandbox with a board and press ok.` のメッセージが出てきたら砂場の上から板を被せてokを押してください
- 板の上にチェスボードが投影されて，サイドキャリブレーションが行われます．

StatusにCalibration succesfulが表示されたら，キャリブレーションは成功です．

#### Debug mode for calibration
キャリブレーションを行なって失敗した場合は以下のフォルダーを参照してください **data\DebugFiles**
ここにキャリブレーション時の画像が保存されていて，失敗した要素を見ることができます．**advanced|Dump Debug** を有効化して，再度キャリブレーションを行なってください．

## Starting the Application
もしキャリブレーションが終了したら **Run** のボタンをおして実行をすることができます．

フレームレートは60 FPS近いPCを使用してください．

## Sandbox games

## RealSense2 対応版では有効化しておりません．
There are a few games included in Magic-Sand

### Shape an Island
The background for the game is that Denmark have more than [400 Islands](https://en.wikipedia.org/wiki/List_of_islands_of_Denmark) and we wanted to create a game that could teach people about islands. We added a few non-Danish Islands as well.
The aim of the game is to shape an Island that matches a given Island. Only the outer contour of the Island is compared (not the height). The game is played like this:

- Press **space** to start the game - now a welcome screen appear
- Shape a big island in the middle of the box (you have 30 seconds). It must NOT be connected to the edge of the box.
- The application will check if an Island exists and if not will let you try again
- Now a name of a given Island is given and you are to shape that specific Island (Australia for example)
- After 30 seconds the application will match your Island with the given Island and show you the results and a score
- Now you have 1 minute to reshape your Island
- Yet another intermediate results will be shown
- Now you have 30 seconds to finalise your Island
- A final result is computed and your score is compared to the overall high-score

It is possible to add more Islands.  Instructions will be added later.

This game was used in an educational event at the Danish Island Bornholm as seen in this [video](https://www.youtube.com/watch?v=dDMrxtH1hyU).

This game was mainly developed by Rasmus R. Paulsen.

### The Sandimals 2-player game
In this game the box is divided into two halfes where each player is only allowed to move the sand in his half. The goal is collect as much **food** and as many **skins** as possible in **5 minutes**. You get **skins** by having rabbits on your half and you get **food** by having fish on your half. The more rabbit the more skins per second. The more and bigger fish the more food per second.

Before starting the game you should flatten the sand and shape a big lake in the middle of the box.

The game is started by pressing **f** on the keyboard. After 5 minutes the game stop and the player who has the most **food+skins** wins the game.

You can also start the game by pressing **1** (complete beginner), **2** (novice), **3** (standard) and **4** (expert).

The behaviour of **fish**:
- fish are light gray
- there is always a fixed number of fish (10, 20, 30 depending on level) in the box
- their speed is determined by their size
- they move in flocks
- they flee from sharks (they will get a **red** center)
- they have a limited life-span (they have a **grey** center when they are soon dying)
- they respawn at the current mother fish
- if a fish is eaten by a shark it respawns as a baby fish by the current mother fish
- the mother fish has a **pink** center
- the current mother fish is the oldest fish in the box
- if a fish ends on land it will seek downwards to find water

The behaviour of **sharks**:
- the shark is white
- there is always 2 sharks in the box
- they move slow and have a **white** stomach when they are well fed
- they get a **black** stomach when they are hungry but not hunting
- they get a **red** stomach when they are hunting and will move fast
- there is a chance they will start hunting when they are hungry
- they will hunt the biggest fish that are close to them
- if they catch the fish they eat it and their hunger decrease.
- if the hunger get too much (if they are trapped) they will die
- if a shark die a new shark respawns
- a shark respawns where there is the highest density of fish
- if they end on land they will seek downwards to find water

The behaviour of **rabbits**:
- there is always a fixed number (4, 6 or 10 depending on level) of rabbits in the box
- they move and stop
- they live on land

Fish and sharks can be taken and moved using your hands if you shape them like a bowl.

This game was mainly developed by Rasmus R. Paulsen.

### The animal and their mothers game

A mother fish and a mother rabbit can be enabled. The user can help the animals to reach their mothers by digging rivers or building mountains in the sand.

The game is started by pressing **m** on the keyboard.

This game was mainly developed by Thomas Wolfe.

## Coding and Extending Magic Sand

### Source Code
このソフトウェアのベースとなった Kinect 用のソースコードは以下のリンクになります．
 [github.com/thomwolf/Magic-Sand](https://github.com/thomwolf/Magic-Sand).
RealSense2 版はこちら

### Create environment

#### Intel RealSense2 SDKのクローン
RealSense2でこのソフトウェアをデバッグする際に，外部のSDKなどが必要になります．以下のコマンドを用いてSDKを導入します．
あらかじめbrewは入っているものとします．
```
brew install ghq # get ghq package
ghq get IntelRealSense/librealsense # clone IntelRealSense/librealsense

# get needs package
brew install libusb pkg-config
brew install glfw
brew install cmake

# setup Build
cd ~/.ghq/github.com/IntelRealSense/librealsense # move library
git reset --hard 719b0b9 # change version
mkdir build && cd build # make build dir and move build
sudo xcode-select --reset # init xcode-select
cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode # cmake librealsense

# Open
open open librealsense2.xcodeproj
```

#### RealSense用のdylibの導入

 - Xcodeで開いたプロジェクトのビルドターゲットを`realsense-viewer`に変更して，実行してください．

 - 実行できたら，ビルドターゲットを`import`に変更して，実行してください．

 - その後`/usr/local/lib/librealsense2.2.dylib`があることを確認してください．

#### XcodeのSDKへのパス

 - クローンしたMagic SandのBuildSettingでOTHERと検索

 - その後，`OTHER LINKER FLAGS`の中に`/usr/local/lib/librealsense2.2.dylib`のパスを追加．

### Dependencies
このMagic Sand for RealSense2 は [openframeworks](http://openframeworks.cc/) release 0.9.8 を使用しています．また，Kinect版 [github.com/thomwolf/Magic-Sand](https://github.com/thomwolf/Magic-Sand) がベースとなっています．
以下のaddonが必要になります．
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
-  [openframeworks](http://openframeworks.cc/download/) から0.9.8版のopenFrameworksをダウンロードしてください．
-  ダウンロードしたopenFrameworksの中に　**app/myApps** で自分用のディレクトリを作成してください．
- インストールしたaddonをopenFrameworks内の **addons** に移動させてください．
- 以下のIDEでプロジェクトを開いてください. (Xcode / VS2015 project files are supplied, should work also on linux)

まだ [openframeworks](http://openframeworks.cc/) のドキュメントやフォーラムを見ながらコードの編集を行うとよいです．充実しています

### How it can be used
このコードは簡単にゲームアプリケーションなどを追加，拡張できるようにデザインされています．

以下にスクリプトの一部を載せていきます．

`Rs2Projector` クラスはRealSense2センサーとのやりとりを行うスクリプトで，キャリブレーションやRealSense2，実際の空間，プロジェクターの変換や連動をおこなうシステムである．

`Rs2Projector`オブジェクトは`setup()`のなかでshared_ptrとして作られています．そのオブジェクトはプロジェクターが投影しているウィンドウのポインタを必要とします．(`main.cpp`では二つのウィンドウが設定されていて，そこでプロジェクターのポインタを取得しています．)

`Rs2Projector`オブジェクトは深さを取得するためや，変換関数を参照するために複数のオブジェクトの変数を共有しています．(マルチスレッドの検証はしていません)

`SandSurfaceRenderer`のオブジェクトは`Rs2Projector`のポインタを使って構成されるものです．(`SandSurfaceRenderer`クラスは深さのデータを砂へ投影するカラーマップへと変換を行います．)

`setup()` 関数では以下のようなものを読み込んでいます:
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
`setup(true)` が呼ばれると `rs2Projector` と `sandSurfaceRenderer` のGUIが表示されることを示しています.

`kinectProjector` オブジェクトはopenFrameworksの標準関数である `update()` で更新をすることが必要になります．その後 `draw()`でプロジェクターに描画します．

`kinectProjector` オブジェクトは `kinectProjector->drawProjectorWindow()`を読んだ後，キャリブレーション時にプロジェクターが投影しているウィンドウを占有してしまうため，プロジェクターウィンドウには何も描画しないように注意してください．  現在キャリブレーションが行われているならば，その状態を`kinectProjector->isCalibrating()`で確認できます.

`update()` や `draw()` がシンプルなサンドボックスの実装をしてくれています． `rs2Projector` と `sandSurfaceRenderer` オブジェクトはプロジェクターウィンドウが`drawProjWindow(ofEventArgs &args)`をコールバックで読んだ際に，内部は初期化されています．(`main.cpp`を参照してください).

```
void ofApp::update(){
  kinectProjector->update();
  sandSurfaceRenderer->update();
}
void ofApp::drawProjWindow(ofEventArgs &args){
  kinectProjector->drawProjectorWindow();

  if (!kinectProjector->isCalibrating()){
      sandSurfaceRenderer->drawProjectorWindow();
      fboVehicles.draw(0,0);
  }
}
```


### rs2Projector Functions

#### Shader functions
`sandSurfaceRenderer` クラスは色が計算されたシェーダーを表示します．

`Rs2Projector` は4x4の行列のユニフォームを設定してます．

```
void bind();
void unbind();
ofMatrix4x4 getTransposedRs2WorldMatrix();
ofMatrix4x4 getTransposedRs2ProjMatrix();
```
`sampler2DRect` はバイナリ化されたシェーダーを受け取り，結果としてそれを変換したあとのデータを送ります．

#### Coordinate conversion / elevation functions
3つの連動させるシステムは以下のように使用されています．:
- RealSense2 2Dイメージに関する連動システム : ピクセル内の(x,y)座標の原点を左上端に変換する処理をおこなっています．
- 現実空間と連動するシステム : xyz座標のRealSense2から取得される㎜単位の座標値を座標データのxyz軸データを拡張しています．
- プロジェクターの連動システム : xy座標のピクセルデータを左上端を原点に変換する処理を行なっています．

The most straighforward conversion goes from kinect coordinates to world coordinate system and projector coordinate system.
If you want to animate or display objects, a natural choice would thus be to store then in kinect coordinate and to perform the conversion on display.

以下の関数が連携システムの変換を行うものです:
```
ofVec2f worldCoordToProjCoord(ofVec3f vin);
ofVec3f projCoordAndWorldZToWorldCoord(float projX, float projY, float worldZ);
ofVec2f rs2CoordToProjCoord(float x, float y);
ofVec3f rs2CoordToWorldCoord(float x, float y);
ofVec2f worldCoordTokinectCoord(ofVec3f wc);
```

`elevation` は，三次元空間でどれくらい離れているかを連動しているものです:
- a normal (`getBasePlaneNormal()`) and an offset (`getBasePlaneOffset()`), or
- a plane equation (`getBasePlaneEq()`).

`elevation` can be converted/accessed by the following functions:
```
float elevationAtKinectCoord(float x, float y);
float elevationToKinectDepth(float elevation, float x, float y);
```

`KinectProjector` also store a matrix of gradients of the kinect depth in the world coordinate system (slope of the sand) computed with a given resolution (with a 10 pixels bin by default).
The gradient at a given location can be accessed by:
```
ofVec2f gradientAtKinectCoord(float x, float y);
```

#### Setup & calibration functions
`startFullCalibration()` perfoms an automatic calibration of the kinect and the projector.
An automatic calibration comprises:
- ask the user to flatten the sand,
- measure the average plane formed by the sand surface to define the base plane (see above),
- display and find 5 chess boards (60 calibration points) on the sand surface,
- ask the user to cover the sand with a board,
- display and find 5 chess boards (60 calibration points) on the board surface,
- set the detection ceiling to 50 milimeters above the board.

The following functions can be called to change some internal values of `kinectProjector`:
- `setGradFieldResolution(int gradFieldResolution)`: change the resolution of the gradient field
- `setSpatialFiltering(bool sspatialFiltering)`: toggle the spatial filtering of the depth frame
- `setFollowBigChanges(bool sfollowBigChanges)`: toggle "big change" detection (follow the hand of the user).

#### Kinect projector state functions

The following functions give information of the state of the kinectprojector object:
- `isCalibrating()`: is the `kinectProjector` currently performing a calibration
- `isCalibrated()`: is the `kinectProjector` calibrated (calibration file found or calibration performed)
- `isImageStabilized()`: is the depth frame stabilized (arbitrary time frame after initialisation)
- `isBasePlaneUpdated()`: was the base plane updated in the previous call to `update()'
- `isROIUpdated()`: was the sand region location/extension updated in the previous call to `update()'
- `isCalibrationUpdated()`: was the calibration updated in the previous call to `update()'

#### Kinect projector other getters
The following functions give additional information :
- `getKinectROI()`: get the sand region location/extension
- `getKinectRes()`: get the kinect resolution
- `getBasePlaneNormal()` : see above
- `getBasePlaneOffset()` : see above
- `getBasePlaneEq()` : see above

## Main differences with [SARndbox](https://github.com/KeckCAVES/SARndbox)

Magic Sand is a cross-platform project while SARndbox currently is only Linux. SARndbox is inherited from a larger VR toolbox that makes is somewhat daunting to start modifying. We hope that Magic Sand is slightly easier to start with.

Magic Sand uses the build-in registration feature of the kinect to perform an automatic calibration between the projector and the kinect sensor and does not use a pixel based depth calibration.

It is thus probably less acurate than SARndbox.

Magic Sand does not provide dynamic rain features (typically require a stronger GPU than the graphic card provided on a laptop).

# Changelog
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
