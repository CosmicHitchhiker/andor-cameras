#include "virtual_camera.h"

using namespace std;
using namespace libconfig;

VirtualCamera::VirtualCamera() : Camera(false){
  char* Model = (char *)"TEST_ANDOR";
  model = Model;

  isInternalShutter = 0; // Checking existance of internal shutter
  width = 2048;
  height = 512;
  getShiftSpeedsInfo();
  hssNo = min_hss_No;
  vssNo = min_vss_No;
  minT = -70;
  maxT = -10;
  temperature = 0.5;
  targetTemperature = int(temperature);
}

void VirtualCamera::init(Log* logFile, Config* ini){
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

  readIni(ini);
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

void VirtualCamera::image(){
  string name = fileName();

  float exposure, accumulate, kinetic;
  exposure = exposureTime;
  accumulate = exposureTime + height*width/hss.at(hssNo) + height*width*vss.at(vssNo)/1000.;
  kinetic = accumulate;
  log->print("Exposure time is %g, accumulate is %g, kinetic is %g", exposure, accumulate, kinetic);
  log->print("Starting acquisition");

  fitsfile *infptr, *outfptr;   /* FITS file pointers defined in fitsio.h */
  int fstatus = 0;       /* status must always be initialized = 0  */
  fits_open_file(&infptr, "sample.fits", READONLY, &fstatus);
  
  sleep(exposureTime);
  log->print("Image is acquired");

  log->print("Saving file %s", name.c_str());

  fits_create_file(&outfptr, (char *)name.c_str(), &fstatus);
  fits_copy_file(infptr, outfptr, 1, 1, 1, &fstatus);
  fits_close_file(outfptr,  &fstatus);
  fits_close_file(infptr, &fstatus);

  log->print("Draft fits is saved.");
  header.update(name);  // Write additional header keys
  log->print("Result fits is saved.");
}


void VirtualCamera::setTemperature(){
  if (targetTemperature < minT) {
    targetTemperature = minT;
    log->print("Target temperature is set to %d", targetTemperature);
  }
  else if (targetTemperature > maxT) {
    targetTemperature = maxT;
    log->print("Target temperature is set to %d", targetTemperature);
  }
  temperature = targetTemperature;  
}


void VirtualCamera::setShutterMode(){
  if (shutterMode > 4 || shutterMode < 0) shutterMode = 0;
  if (isInternalShutter){
    log->print("Internal shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  } else {
    log->print("External shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  }

}
