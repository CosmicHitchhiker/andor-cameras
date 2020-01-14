#include <fstream>
#include <string>
#include <vector>
#include "atmcdLXd.h"
#include "log.h"

#ifndef CAMERA_H
#define CAMERA_H


class Camera
{
  public:
    Camera(bool isParent = true);
    ~Camera(){}
    virtual void init(Log* logFile);
    // virtual void setTemperature(int T);
    // virtual float getTemperature();
    // float getTargetTemperature();
    std::string getModel();

  protected:
    virtual void getShiftSpeedsInfo();
    void andorInit();

  protected:
    float temperature;

    int targetTemperature;
    float exposureTime;
    int hssNo;
    int min_hss_No;
    int vssNo;
    int min_vss_No;
    int readMode;
    int acquisitionMode;
    int shutterMode;
    std::string model;
    int isInternalShutter;
    int shutterCloseT;
    int shutterOpenT;

    std::string writeDirectory;
    std::string prefix;
    std::string postfix;

    Log* log;

    std::vector<std::string> readModes;
    std::vector<std::string> acquisitionModes;
    std::vector<std::string> shutterModes;
    std::vector<float> hss;
    std::vector<float> vss;

};

#endif // CAMERA_H