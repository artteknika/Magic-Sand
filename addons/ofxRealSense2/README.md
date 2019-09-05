# ofxRealSense2
openFrameworksでRealSenseを制御するためのaddonです。

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

