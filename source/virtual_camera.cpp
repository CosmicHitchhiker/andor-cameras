#include "virtual_camera.h"

using namespace std;

VirtualCamera::VirtualCamera() : Camera(false){
  char* Model = "TEST_ANDOR";
  model = Model;

  isInternalShutter = 0; // Checking existance of internal shutter

  readModes = {"Full Vertical Binnig", "Multi-Track", "Random-Track", "Single-Track", "Image"};
  acquisitionModes = {"Single Scan", "Accumulate", "Kinetics", "Fast Kinetics", "Run till abort"};
  shutterModes = {"Fully Auto", "Permanentely Open", "Permanentely Closed", "Open for FVB series", "Open for any series"};
  readMode = 4;
  acquisitionMode = 1;
  exposureTime = 0.1;
  shutterMode = 1;
  shutterOpenT = 50;
  shutterCloseT = 50;
  getShiftSpeedsInfo();
  hssNo = min_hss_No;
  vssNo = min_vss_No;
}

void VirtualCamera::init(Log* logFile){
  log = logFile;

  log->print("Camera %s is initialized.", model.c_str());

  //Set Read Mode to --Image--
  log->print("Read mode is set to '%s'.", readModes.at(readMode).c_str());

  //Set Acquisition mode to --Single scan--
  log->print("Acquisition mode is set to '%s'.", acquisitionModes.at(acquisitionMode).c_str());

  //Set initial exposure time
  log->print("Exposure time is %g.", exposureTime);

  //Initialize Shutter
  if (isInternalShutter){
    // TTL to open is high (1), mode is fully-auto, time to open and close is about 50ms
    log->print("Internal shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  } else {
    // TTL to open is high (1), mode is fully-auto,
    // time to open and close is about 50ms, external shutter mode is fully-auto
  	log->print("External shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  }

  log->print("Horizontal Shift Speed is set to %gMHz", hss.at(hssNo));

  log->print("Vertical Shift Speed is set to %gus", vss.at(vssNo));
}

void VirtualCamera::getShiftSpeedsInfo(){
  /* Заполняет hss и vss, min_hss_No и min_vss_No */
  vector<float> hss_virt = {3, 1, 0.05};
  vector<float> vss_virt = {14, 34, 54}; 
  int NumberOfSpeeds;
  min_hss_No=0;
  float speed, minSpeed=1000;   // minSpeed в МГц, поэтому ставим заведомо большое число

  NumberOfSpeeds = hss_virt.size();
  for (int j=0; j < NumberOfSpeeds; j++){
    speed = hss_virt.at(j);
    hss.push_back(speed);
    if (speed < minSpeed){
      min_hss_No = j;
      minSpeed = speed;
    }
  }

  min_vss_No = 0;
  minSpeed = 0;   // minSpeed в мкс, поэтому ставим заведомо маленькое значение

  NumberOfSpeeds = vss_virt.size();
  for (int j=0; j < NumberOfSpeeds; j++){
    speed = vss_virt.at(j);
    vss.push_back(speed);
    if (speed > minSpeed){
      min_vss_No = j;
      minSpeed = speed;
    }
  }
}