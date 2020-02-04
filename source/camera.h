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
    std::string parseCommand(std::string message);
    /** Return 
      0 if exposure in progress
      1 if exposure finished and image was saved
    */
    virtual bool imageReady();
    virtual std::string saveImage();
    virtual void updateStatement();
    virtual void endWork();
    int expStarted() { return expstarted;}

  protected:
    virtual void getShiftSpeedsInfo();
    virtual std::string startExposure();
    virtual void setTemperature();
    virtual void setShutterMode();
    void andorInit();
    std::string fileName();
    void readIni(Config *ini);
    void bin(int hbin, int vbin);
    std::string textStatus(int);
    virtual void speed(std::string sp);
    virtual void vspeed(std::string sp);

  protected:
    int status, expstarted;
    time_t startTime;
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
    int max_hss_No;
    int vssNo;
    int min_vss_No;
    int max_vss_No;

    int readMode;
    int acquisitionMode;
    int shutterMode;
    std::string model;
    std::string fname;
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
    std::vector<std::string> shutterMStatus;
    std::vector<float> hss;
    std::vector<float> vss;

};

#endif // CAMERA_H
