# Magic Sand for RealSense

Magic Sand for RealSense は [UC Davis](http://idav.ucdavis.edu/~okreylos/ResDev/SARndbox/) によって開発された[Augmented Reality Sandbox](https://arsandbox.ucdavis.edu) のような拡張現実の砂箱を操作するソフトウェアです。

このプロジェクトは [Magic-Sand](https://github.com/thomwolf/Magic-Sand) をベースに、Kinectの代わりに[Intel RealSense D435](https://www.intelrealsense.com/depth-camera-d435/)をセンシングデバイスとして動作するように作られています。

以下の環境で動作を確認しています。
- 実行環境
  - macOS 10.14.6 Mojave
  - openFrameworks 0.9.8
  - Xcode version 9.4.1
  - [RealSense SDK](https://github.com/IntelRealSense/librealsense) version 2.27.0
- RealSense
  - Intel RealSense D435 (Firmware 5.11.11.100)

## Main Features

家庭用プロジェクターとRealSenseセンサーに接続されたコンピュータ上で動作します。
このソフトウェアはRealSenseセンサーで測定された砂の高さに応じた色をプロジェクターで投影するするように制御し、砂場をカラフルな遊び場へと変化させます。

## Getting started

砂箱作成や設定について

[tutorial page](https://imgur.com/gallery/Q86wR) に設定のことについて記述されています。  
[reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/) も参考にしてください。

また、[tutorial page](https://imgur.com/gallery/Q86wR)と[reddit thread](https://www.reddit.com/r/DIY/comments/4v1gfi/a_magic_sandbox_i_made_for_my_3_yo_sons_birthday/)はKinectで行なっていますが、このバージョンはRealSenseで動作します。

### Setting up the system

プロジェクターとRealSenseをPCに繋げて、magic-sandを実行します。

デフォルトでは、ソフトウェアは **setup** モードで起動します。  
このモードでは、RealSenseの深度またはカラーイメージがユーザーインターフェイスに表示され、プロジェクターは白いイメージを投影します。  
RealSense とプロジェクターのステータスは、ユーザーインターフェースの左下の Status ウィンドウに表示されます。もしStatusの項目が表示されない場合は、メインウィンドウのサイズを変更すると表示されます。  
Status ウィンドウのパラメータは以下のようになっています。
- アプリケーションの状態
- RealSenseが認識できているかの状態
- キャリブレーション時のROIが定義されているかの状態
- ベースプレーンが認識できているかの状態
- RealSenseがキャリブレーションできているかの状態
- キャリブレーションのステップ
- キャリブレーション時のエラー回数
- プロジェクターの解像度

**setup** モードでは、RealSense とプロジェクターの物理的位置を最適化することができます。

### Calibration

キャリブレーションを行う際にRealSenseとプロジェクターにいくつか手順が必要です。
- 投影する砂箱の中の砂を平坦にします。
- RealSense から深度イメージとカラーイメージが表示されているかを確認します。 **advanced|Display Rs2 Depth View** で確認します。
- **Calibration|Manually Draw Sand Region** を押します。
- カラーイメージもしくは深度イメージのどちらかでキャリブレーションの領域をマウスで矩形を描いて定義します。
- **Automatically Calibrate Rs2 & Projector** を押します。砂の上にチェスボードが投影され一連のキャリブレーションが行われます。
- `Please cover the sandbox with a board and press ok.` のメッセージが表示されたら、砂の上から薄い板などで覆い、 ok を押します。
- 板の上にチェスボードが投影され、再度キャリブレーションが行われます。

Status に Calibration successful と表示されれば、キャリブレーションは成功です。
ready for calibration のStatus が No の時にキャリブレーションを行った場合に、画像の保存のポップアップが出てきます。

#### Debug mode for calibration

キャリブレーションが成功しなかった場合は、 デバッグモードを有効にして、**data/DebugFiles** フォルダーを参照してください。  
ここにはキャリブレーション時の画像が保存されていて、失敗した要素を見ることができます。これを行うには、**advanced|Dump Debug** を有効化して、再度キャリブレーションを実行してください。

## Starting the Application

キャリブレーションが成功、または以前にキャリブレーションが成功した場合、スペースキーを押すか、**Run** ボタンを押すことでアプリケーションを開始できます。  
これで、等高線と色付きの地図が砂の上に表示されます。最近のPCでは、フレームレートは60fps近く出るはずです。

### Caution
アプリケーションを終了した際に、RealSenseがアプリケーションまたは、RealSense Viewer によってロックされたままになることがあります。
その場合、RealSenseとの接続を切断することによって、ロックを解消することができます。

## Sandbox games

RealSense 対応版では有効化してません。  
有効化する場合は、`src/SandSurfaceRenderer/SandSurfaceRenderer.cpp` を変更します。

## Coding and Extending Magic Sand

### Source Code

このソフトウェアのベースとなった Kinect 用のソースコードは以下のリンクにあります。  
[github.com/thomwolf/Magic-Sand](https://github.com/thomwolf/Magic-Sand)  

このソフトウェアは、RealSenseでしか実行することができません。

### Create environment

RealSense用の環境構築手順です。

#### [Tips]

RealSense の SDK は頻繁に変更されています。前述したバージョン以外で実行した場合、動作しないことがあります。

#### Intel RealSense SDKの導入

RealSense でこのソフトウェアをデバッグする際に、外部の SDK などが必要になります。以下のコマンドを用いて導入します。
あらかじめ [Homebrew](https://brew.sh/) がインストールされているものとします。

```
# Clone IntelRealSense/librealsense
git clone https://github.com/IntelRealSense/librealsense.git

# Get needs package
brew install libusb pkg-config
brew install glfw
brew install cmake

# Setup build
cd ./librealsense # move library
git reset --hard 719b0b9 # change version
mkdir build && cd build # make build dir and move build
sudo xcode-select --reset # init xcode-select
cmake .. -DBUILD_EXAMPLES=true -DBUILD_WITH_OPENMP=false -DHWM_OVER_XU=false -G Xcode # cmake librealsense

# Open with Xcode
open librealsense2.xcodeproj
```

#### RealSense用のdylibの導入

- ビルドターゲットを `install` に変更して実行します。
- その後 `/usr/local/lib/librealsense2.dylib` が出来ていることを確認します。

#### XcodeへSDKへのパスを追加

Xcode の Build Settings の `Other Linker Flags` に `/usr/local/lib/librealsense2.dylib` のパスを追加します。
同様に Build Settings の `Header Search Paths` に `/usr/local/include` のパスを追加します。

### Dependencies

このMagic Sand for RealSense は [openframeworks](http://openframeworks.cc/) ver0.9.8 を使用しています。また、Kinect版 [github.com/thomwolf/Magic-Sand](https://github.com/thomwolf/Magic-Sand) がベースとなっています。  
以下のアドオンが必要です。

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
- [openframeworks](http://openframeworks.cc/download/) から0.9.8版のopenFrameworksをダウンロードします。
- ダウンロードしたopenFrameworksの **app/myApps** フォルダーでMagic-Sandを展開します。
- コミュニティーアドオンをopenFrameworks内の **addons** フォルダーに移動します。
- IDEでプロジェクトを開きます。(Xcode / VS2015 project files are supplied, should work also on linux)

Xcode 10 以降の場合、openFrameworksの **libs/openFrameworksCompiled/project/osx/CoreOF.xcconfig** ファイルを修正する必要があります。
**CoreOF.xcconfig** ファイルの`VLID_ARCHS`から`i386`を削除し、`OF_CORE_FRAMEWORKS`から`-framework QuickTime`を削除してください。

まだ不明な場合は [openframeworks](http://openframeworks.cc/) のドキュメントやフォーラムを確認してください。すばらしいコミュニティです！

### How it can be used
このコードは簡単に拡張できるように設計されており、これをベースに追加したいゲーム、アプリケーションを開発できます。

以下の説明の一部は少し古いことに注意してください。

`Rs2Projector`クラスは、RealSenseセンサーとの通信、キャリブレーション、RealSense(2D)座標系・ワールド(3D)座標系・プロジェクター(2D)座標系間の変換を処理します。

openFrameworksの`setup()`関数内で`R2Projector`オブジェクトを`shared_ptr`として作成できます。このオブジェクトはプロジェクターウィンドウへのポインタが必要です。(openFrameworksで2つのウィンドウを適切に設定し、プロジェクターウィンドウへのポインタを取得する方法については、`main.cpp`を参照してください)。

`Rs2Projector`オブジェクトは深度を取得するためや、変換関数を参照するためにさまざまなオブジェクト間で共有できます(マルチスレッドの検証はしていません)。

例えば、`SandSurfaceRenderer`オブジェクトは`Rs2Projector`共有オブジェクトへのポインタを使って生成されます。(`SandSurfaceRenderer`クラスは編集可能なカラーマップを使用して深度情報を色情報へ変換し、砂の上へ表示します)。

openFrameworksの `setup()` 関数は以下のようになります。

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

`setup(true)` が呼ばれると `rs2Projector` と `sandSurfaceRenderer` のGUIが表示されることを示しています。

その後、`rs2Projector`オブジェクトはopenFrameworksの関数である`update()`内で更新され(オブジェクトの関数を使用する前が望ましい)、プロジェクターの`draw()`関数内で描画される必要があります。

`rs2Projector`オブジェクトはキャリプレーション中にプロジェクターウィンドウを占有する必要があるため、`rs2Projector->drawProjectorWindow()`を呼び出した後は、キャリプレーション実行中はプロジェクターウィンドウには何も描画しないように注意してください。(現在キャリブレーションが行われているかは`rs2Projector->isCalibrating()`で確認できます)

上記のように`rs2Projector`と`sandSurfaceRenderer`が初期化され、
プロジェクターウィンドウのリスナーコールバックに`drawProjWindow(ofEventArgs &args)`関数を登録した場合(`main.cpp`を参照)、
シンプルなARサンドボックスの実装は、次の`update()`と`draw()`関数のようになります。

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

### Rs2Projector Functions

#### Shader functions
`sandSurfaceRenderer`クラスでは色の計算に使用可能なシェーダーとユニフォーム変数の設定方法の例を示しています。

以下の`Rs2Projector`の関数は、ユニフォーム変数を準備するために特に重要です。

```
void bind();
void unbind();
ofMatrix4x4 getTransposedRs2WorldMatrix();
ofMatrix4x4 getTransposedRs2ProjMatrix();
```

シェーダーで受け取った`sampler2DRect`は0から1に正規化されているため、変換スケールも送信する必要があります。

#### Coordinate conversion / elevation functions
次の3つの座標系を使用できます。
- 2次元RealSense画像のRealSense座標系 : 左上隅を原点とするピクセル単位の座標(x, y)。
- ワールド座標系 : RealSenseセンサーから伸びたz軸のある、RealSenseセンサーを起点としたミリメートル単位の3D座標系(x, y, z)で、xがRealSenseセンサーの水平軸、yが垂直軸。
- 2次元プロジェクター画像のプロジェクター座標系 : 原点が左上隅のピクセル単位の座標(x, y)。

最もわかりやすい変換は、RealSense座標系からワールド座標系とプロジェクター座標系への変換です。
もしオブジェクトをアニメーションしたり表示したいなら、通常であればRealSense座標を保存し表示上で変換します。

以下の関数が座標系間の変換を提供しています。

```
ofVec2f worldCoordToProjCoord(ofVec3f vin);
ofVec3f projCoordAndWorldZToWorldCoord(float projX, float projY, float worldZ);
ofVec2f rs2CoordToProjCoord(float x, float y);
ofVec3f rs2CoordToWorldCoord(float x, float y);
ofVec2f worldCoordTors2Coord(ofVec3f wc);
```

使用可能なもう1つの値は`elevation`(標高)です。これはワールド座標上の点から次のように定義される3次元ベース平面までの距離です。

- 法線 (`getBasePlaneNormal()`)とオフセット (`getBasePlaneOffset()`)
- 平面方程式(`getBasePlaneEq()`).

`elevation` は以下の関数で変換/取得をすることができます。
```
float elevationAtRs2Coord(float x, float y);
float elevationToRs2Depth(float elevation, float x, float y);
```

`Rs2Projector`は、指定された解像度(デフォルトは10ピクセル)でRealSense深度の勾配行列をワールド座標系で格納します。
指定の位置の勾配は次の方法で取得可能です。

```
ofVec2f gradientAtRs2Coord(float x, float y);
```

#### Setup & calibration functions
`startFullCalibration()`は、プロジェクターとrealsenseの自動キャリブレーションを行います。
自動キャリブレーションの処理は以下で構成されます。
- ユーザに砂を平らにするように求める。
- 基準平面を定義するために、平均平面を砂の表面から測定する(下記参照)。
- 砂の表面に5つのチェスボード(60のキャリブレーションポイント)を表示し、認識させる。
- ユーザに砂の上に板をかぶせるように求める。
- 砂の表面に10個のチェスボード(60のキャリブレーションポイント)を表示し、認識させる。
- 板の50mm上が検出上限に設定される。

以下の関数と呼ぶことで、`rs2Projector`の内部値を変更することができます。
- `setGradFieldResolution(int gradFieldResolution)`: 勾配場の解像度を変更。
- `setSpatialFiltering(bool sspatialFiltering)`: 深度フレームの空間フィルタリングを切り替え。
- `setFollowBigChanges(bool sfollowBigChanges)`: "大きな変化"の検出を切り替え(ユーザの手に追従)。

#### Rs2 projector state functions

以下の関数は`rs2Projector`オブジェクトの状態の情報を提供します。
- `isCalibrating()`: `rs2Projector`が現在キャリブレーションを実行しているかを取得します。
- `isCalibrated()`: `rs2Projector`がキャリブレーションをされているかを取得します。(キャリブレーションファイルが見つかったか、またはキャリブレーションが実行されたか)
- `isImageStabilized()`: 深度フレームが安定しているかを取得します(初期化後の任意の時間フレーム)。この値によってキャリブレーションの準備ができているかを判断できます。
- `isBasePlaneUpdated()`: 前回の`update()`呼び出しで基準平面が更新をされているかを取得します。
- `isROIUpdated()`: 前回の`update()`呼び出しでROIが更新をされているかを取得します。
- `isCalibrationUpdated()`: 前回の`update()`呼び出しでキャリブレーションの状態が更新をされているかを取得します。

#### Rs2 projector other getters
以下の関数は`rs2Projector`の追加情報を提供します。
- `getRs2ROI()`: 砂のROIを取得します。
- `getRs2Res()`: RealSenseの解像度を取得します。
- `getBasePlaneNormal()` : 上記を参照。
- `getBasePlaneOffset()` : 上記を参照。
- `getBasePlaneEq()` : 上記を参照。

## points to change

### ファイル、ディレクトリ名前の変更点
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
## [1.5.4.1 for RealSense](https://github.com/artteknika/Magic-Sand-for-RealSense/releases/tag/v1.5.4.1-for-RealSense) - 05-09-2019
Supported RealSense SDK 2

### Changed
- refer to Point to change
- We can run this software of RealSense

## [1.5.4.1](https://github.com/thomwolf/Magic-Sand/releases/tag/v1.5.4.1) - 10-10-2017
Bug fix release

### Bug fixes
- The calibration procedure was broken in 1.5.4 - it did not show the checkerboard. Now fixed。

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
