#include <stdio.h>
#include <opencv2/opencv.hpp>

int main(void){
  cv::Mat a = cv::imread("~/OF/of_v0.9.8_osx_release/apps/magic-sand/bin/tmp/img.png", 1);
  cv::Mat nlMeans,adaptiveBilateral,median;
  cv::fastNlMeansDenoising(a, adaptiveBilateral, 3, 3, 7);
  cv::imwrite("~/OF/of_v0.9.8_osx_release/apps/magic-sand/bin/tmp/img.png",a);
  return 0;
}
