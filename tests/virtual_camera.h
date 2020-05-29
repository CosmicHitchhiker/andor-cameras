#include "../source/camera.h"

using namespace libconfig;

#ifndef VIRTUAL_CAMERA_H
#define VIRTUAL_CAMERA_H


class VirtualCamera : public Camera
{
  public:
    VirtualCamera();
    ~VirtualCamera(){}
    int init(Log* logFile, std::string iniName);
    void updateStatement();
    void endWork();
    bool imageReady();
    std::string saveImage();

  protected:
    void getShiftSpeedsInfo();
    std::string startExposure();
    void image();
    void setTemperature();
    void setShutterMode();
    void speed(std::string sp);
    void vspeed(std::string sp);
};

#endif // VIRTUAL_CAMERA_H