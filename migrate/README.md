# 相关第三方库移植文档

## OpenCV

仓库地址：https://gitee.com/openharmony-sig/third_party_opencv/
该仓库已经移植

## SeetaFace2

原仓库：https://github.com/seetafaceengine/SeetaFace2
移植文件：./SeetaFace2/
维护：
```bash
cd third_party/SeetaFace2/
cp --parents ./bundle.json ./BUILD.gn ./SeetaNet/BUILD.gn ./FaceRecognizer/BUILD.gn ./QualityAssessor/BUILD.gn ./FaceDetector/BUILD.gn ./FaceTracker/BUILD.gn ./FaceLandmarker/BUILD.gn /home/user/OpenHarmony4.0Release/sample/guardsys/migrate/SeetaFace2
```