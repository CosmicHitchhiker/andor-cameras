#include "../source/camera.h"

using namespace libconfig;

#ifndef VIRTUAL_CAMERA_H
#define VIRTUAL_CAMERA_H


class VirtualCamera : public Camera
{
  public:
    VirtualCamera();
    ~VirtualCamera() {}
    void init(Log* logFile, Config* ini);
    void updateStatement();
    void endWork();
    // void setTemperature(int T);
    // float getTemperature();

  protected:
    void getShiftSpeedsInfo();
    void image();
    void setTemperature();
    void setShutterMode();
};

#endif // VIRTUAL_CAMERA_H