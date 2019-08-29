# ofxRealSense2
openFrameworks の環境下でRealSenseを動かすためのaddonになります。
RealSense SDK が不安定なので推奨するバージョン以外で行った場合に正常な動作をしない場合があります。

動作確認された環境は以下のようになります。

* 開発環境
    * macOS 10.13 High Sierra
    * Xcode ver 9.3
    * [Intel RealSense SDK 2.0](https://github.com/IntelRealSense/librealsense) (build 2.13.0)
    * Intel RealSense D435

## 環境構築

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

### RealSense用のdylibの導入

- Xcodeで開いたプロジェクトのビルドターゲットを `install` に変更して実行します。
- その後 `/usr/local/lib/librealsense2.2.dylib` が出来ていることを確認します。

### XcodeへSDKへのパスを追加

Xcode の Build Settings の `Other Linker Flags` に `/usr/local/lib/librealsense2.2.dylib` のパスを追加します。

## `ofxRealSense2`の関数

### RealSenseとの接続、デバイスの初期化、初期化、切断、状態

* `init()` : ofxRealSense2とデバイスを初期化します。(必ず`open()`をする前にこの関数を呼んでください。)
* `clear()` : デバイスのリソースの初期化を行います。(`init()`と違いofxRealSense2が起動している時に呼び出してください。)
* `open()` : デバイスとの接続を行います。
* `close()` : デバイスとの接続を切断します。
* `isConnected()` : デバイスが起動しているかを取得します。
* `isInitialized()` : デバイスが初期化されているかを取得します。

### フレームの取得や更新、状態を取得する関数

* `getRawDepthPixels()` : RealSenseが取得している未加工の深度情報を取得します。
* `getDepthPixels()` : 一定の値を超えた深度の値を0にした深度情報を取得します。
* `getPixels()` : カラーイメージを取得します。
* `isFrameNew()` : 現在のフレームが最新のものかを取得します。
* `update` : 現在のフレームから新しいフレームを取得し更新します。

