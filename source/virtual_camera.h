#include "camera.h"

#ifndef VIRTUAL_CAMERA_H
#define VIRTUAL_CAMERA_H


class VirtualCamera : public Camera
{
  public:
    VirtualCamera();
    ~VirtualCamera() {}
    void init(Log* logFile);
    // void setTemperature(int T);
    // float getTemperature();

  protected:
    void getShiftSpeedsInfo();
    void image();
};

#endif // VIRTUAL_CAMERA_H