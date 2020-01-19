#include <fstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
// #include <functional>
#include <numeric>
#include "atmcdLXd.h"
#include "log.h"
#include "header_values.h"
#include <ctime>

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
    void parseCommand(std::string message);

  protected:
    virtual void getShiftSpeedsInfo();
    virtual void image();
    void andorInit();
    std::string fileName();

  protected:
    int status;

    float temperature;
    int targetTemperature;

    float exposureTime;

    int width;
    int height;
    int hBin;
    int vBin;
    std::vector<int> crop;

    int hssNo;
    int min_hss_No;
    int vssNo;
    int min_vss_No;

    int readMode;
    int acquisitionMode;
    int shutterMode;
    std::string model;
    int isInternalShutter;
    int shutterCloseTime;
    int shutterOpenTime;

    std::string writeDirectory;
    std::string prefix;
    std::string postfix;
    int dataType;

    HeaderValues header;
    Log* log;

    std::vector<std::string> readModes;
    std::vector<std::string> acquisitionModes;
    std::vector<std::string> shutterModes;
    std::vector<float> hss;
    std::vector<float> vss;

};

#endif // CAMERA_H