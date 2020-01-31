#include "../source/camera.h"

using namespace libconfig;

#ifndef VIRTUAL_CAMERA_H
#define VIRTUAL_CAMERA_H


class VirtualCamera : public Camera
{
  public:
    VirtualCamera();
    ~VirtualCamera(){}
    void init(Log* logFile, Config* ini);
    void updateStatement();
    void endWork();

  protected:
    void getShiftSpeedsInfo();
    void image();
    void setTemperature();
    void setShutterMode();
    void speed(std::string sp);
    void vspeed(std::string sp);
};

#endif // VIRTUAL_CAMERA_H