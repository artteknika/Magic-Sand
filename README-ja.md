# Magic Sand For RealSense2
Magic Sand For RealSense は [Augmented Reality Sandbox](https://arsandbox.ucdavis.edu) のような砂場をマッピングするソフトウェアです．
開発元 [UC Davis](http://idav.ucdavis.edu/~okreylos/ResDev/SARndbox/).

このプロジェクトは [Magic-Sand](https://github.com/thomwolf/Magic-Sand) をベースに作られています．

この開発環境と実行環境は以下のようになっています．
 - 実行環境
  * 開発者向けノート，デスクトップ(Max OS X High Sierra Ver.10.13 GPU必須 実行確認済み環境)
  * openFrameworks ver.0.9.8
 - 開発環境
  * Max OS X High Sierra Ver.10.13
  * Xcode ver 9.3
  * [RealSense SDK](https://github.com/IntelRealSense/librealsense) ver v2.13.0
 - ハードウェア
  * Intel Real Sense 2 435 (動作確認済み)
## Main Features

プロジェクターとRealSense2をPCに繋げます．
プロジェクターのセンシングを行うために，キャリブレーションを行います．
キャリブレーション成功後，RealSense2から深さのセンシングを行い，そのキャリブレーションのデータを元にプロジェクターに投影します．

## Getting started

砂場作成や設定について

[tutorial page](https://imgur.com/gallery/Q86wR) に設定のことについて記述されています．　
[reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/)　も参考にしてください．

また，[tutorial page](https://imgur.com/gallery/Q86wR)と[reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/)はKinectで行なっていますが，このバージョンはRealSense2で動作します．

### Setting up the system

プロジェクターとRealSense2をPCに繋げて，magic-sandを実行してください．

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

In **setup** mode the physical positions of the RealSense2 and projector can be optimised.

### Calibration

キャリブレーションを行う際にRealSense2とProjectorに必要な設定があります．
- 投影する砂箱を平坦にしてください
- RealSense2 から深さのイメージとカラーイメージがとれているかを確認してください． **advanced|Display Rs2 Depth View** で確認
- **Calibration|Manually Draw Sand Region** を押してください．
- カラーイメージもしくは深さイメージのどちらかでキャリブレーションの範囲を四角形を定義してください．
- **Automatically Calibrate Rs2 & Projector** を押して一連のキャリブレーションの流れを行なってください
- `Please cover the sandbox with a board and press ok.` のメッセージが出てきたら砂場の上から板を被せてokを押してください
- 板の上にチェスボードが投影されて，サイドキャリブレーションが行われます．

StatusにCalibration successfulが表示されたら，キャリブレーションは成功です．

#### Debug mode for calibration
キャリブレーションを行なって失敗した場合は以下のフォルダーを参照してください **data\DebugFiles**
ここにキャリブレーション時の画像が保存されていて，失敗した要素を見ることができます．**advanced|Dump Debug** を有効化して，再度キャリブレーションを行なってください．

## Starting the Application
もしキャリブレーションが終了したら **Run** のボタンを押して実行をすることができます．

フレームレートは60 FPS近いPCを使用してください．

## Sandbox games

### RealSense2 対応版では有効化しておりません．

### 有効化する場合
    * `src/SandSurfaceRenderer/SandSurfaceRenderer.cpp`を変更してください．

## Coding and Extending Magic Sand

### Source Code
このソフトウェアのベースとなった Kinect 用のソースコードは以下のリンクになります．
 [github.com/thomwolf/Magic-Sand](https://github.com/thomwolf/Magic-Sand).
 このバージョンはRealSenseをサポートしています．

### Create environment
RealSense2用の環境構築になります．
#### [Tips]
    * RealSense2 の SDKが不安定なので前述したバージョン以外で実行した場合動かないという場合が起こり得ます．

#### Intel RealSense2 SDKのクローン
RealSense2でこのソフトウェアをデバッグする際に，外部のSDKなどが必要になります．以下のコマンドを用いてSDKを導入します．
あらかじめbrewは入っているものとします．
```
git clone https://github.com/IntelRealSense/librealsense.git # clone IntelRealSense/librealsense

# get needs package
brew install libusb pkg-config
brew install glfw
brew install cmake

# setup Build
cd ./librealsense # move library
git reset --hard 719b0b9 # change version
mkdir build && cd build # make build dir and move build
sudo xcode-select --reset # init xcode-select
cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode # cmake librealsense

# Open
open librealsense2.xcodeproj
```

#### RealSense用のdylibの導入

 - Xcodeで開いたプロジェクトのビルドターゲットを`realsense-viewer`に変更して，実行してください．

 - 実行できたら，ビルドターゲットを`import`に変更して，実行してください．

 - その後`/usr/local/lib/librealsense2.2.dylib`があることを確認してください．

#### XcodeのSDKへのパス

 - クローンしたMagic SandをXcodeで開き magic-sand プロジェクトのBuildSettingで`OTHER Linker Flags`と検索してください

 - その後，`OTHER LINKER FLAGS`の`magic-sand`の中に実行時に読み込むリンカの設定ができるので`/usr/local/lib/librealsense2.2.dylib`のパスを追加してください．

### Dependencies
このMagic Sand for RealSense2 は [openframeworks](http://openframeworks.cc/) ver0.9.8 を使用しています．また，Kinect版 [github.com/thomwolf/Magic-Sand](https://github.com/thomwolf/Magic-Sand) がベースとなっています．
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
このコードは簡単にゲームアプリケーションなどを追加，拡張できるように設計されています．

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

`rs2Projector` オブジェクトはopenFrameworksの標準関数である `update()` で更新をすることが必要になります．その後 `draw()`でプロジェクターに描画します．

`rs2Projector` オブジェクトは `rs2Projector->drawProjectorWindow()`を読んだ後，キャリブレーション時にプロジェクターが投影しているウィンドウを占有してしまうため，プロジェクターウィンドウには何も描画しないように注意してください．  現在キャリブレーションが行われているならば，その状態を`rs2Projector->isCalibrating()`で確認できます.

`update()` や `draw()` がシンプルなサンドボックスの実装をしてくれています． `rs2Projector` と `sandSurfaceRenderer` オブジェクトはプロジェクターウィンドウが`drawProjWindow(ofEventArgs &args)`をコールバックで読んだ際に，内部は初期化されています．(`main.cpp`を参照してください).

```
void ofApp::update(){
  rs2Projector->update();
  sandSurfaceRenderer->update();
}
void ofApp::drawProjWindow(ofEventArgs &args){
  rs2Projector->drawProjectorWindow();

  if (!Projector->isCalibrating()){
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

もっとも簡単なrealsense2から取得する座標をワールド座標とプロジェクターの座標に変換し連動するものです。
もし、アニメーションをするオブジェクトを表示したいならば、普通はこのようにrealsense2の連動された値を格納し、表示の変換を行います。

以下の関数が連携システムの変換を行うものです:

```
ofVec2f worldCoordToProjCoord(ofVec3f vin);
ofVec3f projCoordAndWorldZToWorldCoord(float projX, float projY, float worldZ);
ofVec2f rs2CoordToProjCoord(float x, float y);
ofVec3f rs2CoordToWorldCoord(float x, float y);
ofVec2f worldCoordTors2Coord(ofVec3f wc);
```

`elevation` は，三次元空間でどれくらい離れているかを連動しているものです:
- (`getBasePlaneNormal()`) は面法線を取得し、(`getBasePlaneOffset()`)　は平面の補正値を返します
- (`getBasePlaneEq()`) は平面方程式です。

`elevation` は以下の関数で変換/取得をすることができます。
```
float elevationAtRs2Coord(float x, float y);
float elevationToRs2Depth(float elevation, float x, float y);
```

`Rs2Projector` もまた，realsenseから取得する深さの勾配の値をワールド座標値を連動するものに格納することができ，解像度を用いて計算を行うことができます。
x,yで指定する座標の勾配を取得するのは以下の関数で可能です。
```
ofVec2f gradientAtRs2Coord(float x, float y);
```

#### Setup & calibration functions
`startFullCalibration()` プロジェクターとrealsenseの自動のキャリブレーションを行う関数です。
自動キャリブレーションの処理構造は以下のようになります:
- キャリブレーションを行うユーザに対して砂場が平坦かを確認します。
- 基準平面を定義するために、平均平面を砂場の表面から形作ります。
- 砂場の表面に5つのチェスボードを表示し、認識させます。
- ユーザに砂の上に板をかぶせるように尋ねます。
- 砂場の表面に10個のチェスボードを表示し、認識させます。
- 50mm以上の板を検出します。

`rs2Projector`の以下の関数を呼ぶことによって内部の値を変更することができます。:
- `setGradFieldResolution(int gradFieldResolution)`: 平面の勾配の解像度を変更することができます。
- `setSpatialFiltering(bool sspatialFiltering)`: 深さを取得しているフレームの空間フィルターをトグルで切り替えることができます。
- `setFollowBigChanges(bool sfollowBigChanges)`: 検出の大きな変更ををトグルで切り替えることができます。

#### Rs2 projector state functions

`rs2projector` オブジェクトの状態の情報を以下の関数によって表示することができます。:
- `isCalibrating()`: `rs2tProjector` 現在のキャリブレーションの状態を取得します。
- `isCalibrated()`: `rs2Projector` キャリブレーションをされているかを取得します。
- `isImageStabilized()`: 深さを取得しているフレームが安定しているかを取得します。この値によってキャリブレーションの準備ができているかを取得します。
- `isBasePlaneUpdated()`: `update()`を呼ぶ前に基準平面の更新をします。
- `isROIUpdated()`: `update()` を呼ぶ前に砂のROIを更新します。
- `isCalibrationUpdated()`:  `update()` を呼ぶ前にキャリブレーションの状態を更新します。

#### Rs2 projector other getters
追加の`rs2Projector`の追加の情報を以下の関数によって取得することができます。 :
- `getRs2ROI()`:　砂場のROIを取得することができます。
- `geRs2Res()`: realsense2の解像度を取得することができます。
- `getBasePlaneNormal()` : 上記を参照してください
- `getBasePlaneOffset()` : 上記を参照してください
- `getBasePlaneEq()` : 上記を参照してください

## points to change

### ファイル，ディレクトリ名前の変更点
    * `KinectProjector` -> `Rs2Projector`
        - `KinectGrabber.cpp` -> `Rs2Grabber.cpp`
        - `KinectGrabber.h` -> `Rs2Grabber.h`
        - `KinectProjector.h` -> `Rs2Projector.h`
        - `KinectProjector.cpp` -> `Rs2Projector.cpp`
        - `KinectProjectorCalibration.h` -> `Rs2ProjectorCalibration.h`
        - `KinectProjectorCalibration.cpp` -> `Rs2ProjectorCalibration.cpp`

#### 主要な変更点
    * `Rs2Projector`
        - `Rs2Grabber.cpp`
        - `Rs2Grabber.h`
        - `Rs2Projector.h`
        - `Rs2Projector.cpp`
    * `SandSurfaceRenderer`
        - `SandSurfaceRenderer.cpp`

#### 軽微な変更点
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
