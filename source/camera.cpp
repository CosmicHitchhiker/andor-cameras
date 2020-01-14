#include "camera.h"

using namespace std;

Camera::Camera(bool isParent){
  if (isParent) {
    andorInit();

    char* Model = new char[40];
    GetHeadModel(Model);
    model = Model;

    IsInternalMechanicalShutter(&isInternalShutter); // Checking existance of internal shutter

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
  
}

void Camera::init(Log* logFile){
  log = logFile;

  log->print("Camera %s is initialized.", model.c_str());

  //Set Read Mode to --Image--
  SetReadMode(readMode);
  log->print("Read mode is set to '%s'.", readModes.at(readMode).c_str());

  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode(acquisitionMode);
  log->print("Acquisition mode is set to '%s'.", acquisitionModes.at(acquisitionMode).c_str());

  //Set initial exposure time
  SetExposureTime(exposureTime);
  log->print("Exposure time is %g.", exposureTime);

  //Initialize Shutter
  if (isInternalShutter){
    // TTL to open is high (1), mode is fully-auto, time to open and close is about 50ms
    SetShutter(shutterMode,0,50,50);
    log->print("Internal shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  } else {
    // TTL to open is high (1), mode is fully-auto,
    // time to open and close is about 50ms, external shutter mode is fully-auto
    SetShutterEx(shutterMode, 0, 50, 50, shutterMode);
    log->print("External shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  }

  SetHSSpeed(0, hssNo);
  log->print("Horizontal Shift Speed is set to %gMHz", hss.at(hssNo));

  SetVSSpeed(vssNo);
  log->print("Vertical Shift Speed is set to %gus", vss.at(vssNo));
}

void Camera::andorInit(){
  unsigned long error;
  int NumberOfCameras, CameraHandle, i;

  GetAvailableCameras(&NumberOfCameras);
  for (i = 0; i < NumberOfCameras; i++){
    GetCameraHandle(i, &CameraHandle);
    SetCurrentCamera(CameraHandle);
    error = Initialize("/usr/local/etc/andor");
    if (error == DRV_SUCCESS) break;
  }

  if(i == NumberOfCameras){
    cerr << "Can't find desired camera";
    exit(1);
  }

  sleep(2); //sleep to allow initialization to complete
}

void Camera::getShiftSpeedsInfo(){
  /* Заполняет hss и vss, min_hss_No и min_vss_No */
  int NumberOfSpeeds;
  min_hss_No=0;
  float speed, minSpeed=1000;   // minSpeed в МГц, поэтому ставим заведомо большое число

  GetNumberHSSpeeds(0, 0, &NumberOfSpeeds);
  for (int j=0; j < NumberOfSpeeds; j++){
    GetHSSpeed(0, 0, j, &speed);
    hss.push_back(speed);
    if (speed < minSpeed){
      min_hss_No = j;
      minSpeed = speed;
    }
  }

  min_vss_No = 0;
  minSpeed = 0;   // minSpeed в мкс, поэтому ставим заведомо маленькое значение

  GetNumberVSSpeeds(&NumberOfSpeeds);
  for (int j=0; j < NumberOfSpeeds; j++){
    GetVSSpeed(j, &speed);
    vss.push_back(speed);
    if (speed > minSpeed){
      min_vss_No = j;
      minSpeed = speed;
    }
  }
}

string Camera::getModel(){
  return model;
}

