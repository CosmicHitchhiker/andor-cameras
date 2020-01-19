#include <fstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <numeric>

#include "atmcdLXd.h"
#include <libconfig.h++>

#include "log.h"
#include "header_values.h"

#ifndef CAMERA_H
#define CAMERA_H

using namespace libconfig;


class Camera
{
  public:
    Camera(bool isParent = true);
    ~Camera(){}
    virtual void init(Log* logFile, Config* ini);
    std::string getModel();
    void parseCommand(std::string message);
    virtual void updateStatement();

  protected:
    virtual void getShiftSpeedsInfo();
    virtual void image();
    virtual void setTemperature();
    virtual void setShutterMode();
    void andorInit();
    std::string fileName();
    void readIni(Config *ini);
    virtual void endWork();

  protected:
    int status;

    float temperature;
    int targetTemperature;
    int minT, maxT;
    unsigned int temperatureStatus;

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
    Config cfg;
    std::string configName;

    std::vector<std::string> readModes;
    std::vector<std::string> acquisitionModes;
    std::vector<std::string> shutterModes;
    std::vector<float> hss;
    std::vector<float> vss;

};

#endif // CAMERA_H