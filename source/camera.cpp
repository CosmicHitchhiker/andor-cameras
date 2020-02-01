#include "camera.h"

using namespace std;
using namespace libconfig;

Camera::Camera(bool isParent){
  if (isParent) {
    andorInit();

    char* Model = new char[40];
    GetHeadModel(Model);
    model = Model;
    configName = model + ".info";

    IsInternalMechanicalShutter(&isInternalShutter); // Checking existance of internal shutter
    GetDetector(&width, &height);
    crop = {1,width,1,height};
    getShiftSpeedsInfo();
    hssNo = min_hss_No;
    vssNo = min_vss_No;

    CoolerON();
    GetTemperatureRange(&minT, &maxT);
    GetTemperatureF(&temperature);
    targetTemperature = int(temperature);
    SetTemperature(targetTemperature);
  }
  readModes = {"Full Vertical Binning", "Multi-Track", "Random-Track", "Single-Track", "Image"};
  acquisitionModes = {"Single Scan", "Accumulate", "Kinetics", "Fast Kinetics", "Run till abort"};
  shutterModes = {"Fully Auto", "Permanentely Open", "Permanentely Closed", "Open for FVB series", "Open for any series"};
  readMode = 4;
  acquisitionMode = 0;
  exposureTime = 0.1;
  shutterMode = 0;
  shutterOpenTime = 50;
  shutterCloseTime = 50;
  hBin = 1;
  vBin = 1;
  dataType = 16;

  prefix = "";
  postfix = "";
  writeDirectory = "";

  Setting &root = cfg.getRoot();
  root.add("Temperature", Setting::TypeFloat);
  root.add("TargetT", Setting::TypeInt);
  root.add("CoolingStatus", Setting::TypeString);
  root.add("Shutter", Setting::TypeString);
  // root.add("Acquisition", Setting::TypeString);
  // root.add("Read", Setting::TypeString);
  // root.add("Status", Setting::TypeString);
  // root.add("Prefix", Setting::TypeString);
  // root.add("Postfix", Setting::TypeString);
  // root.add("Dir", Setting::TypeString);
  // root.add("Acquisition", Setting::TypeString);
}


void Camera::init(Log* logFile, Config* ini){
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
    SetShutter(1,shutterMode,shutterOpenTime,shutterCloseTime);
    log->print("Internal shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  } else {
    // TTL to open is high (1), mode is fully-auto,
    // time to open and close is about 50ms, external shutter mode is fully-auto
    SetShutterEx(1,shutterMode, shutterOpenTime, shutterCloseTime, shutterMode);
    log->print("External shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  }

  SetHSSpeed(0, hssNo);
  log->print("Horizontal Shift Speed is set to %gMHz", hss.at(hssNo));

  SetVSSpeed(vssNo);
  log->print("Vertical Shift Speed is set to %gus", vss.at(vssNo));

  readIni(ini);
  setTemperature();
  updateStatement();
}


void Camera::andorInit(){
  unsigned long error;
  int NumberOfCameras, CameraHandle, i;

  GetAvailableCameras(&NumberOfCameras);
  for (i = 0; i < NumberOfCameras; i++){
    GetCameraHandle(i, &CameraHandle);
    SetCurrentCamera(CameraHandle);
    error = Initialize((char *)"/usr/local/etc/andor");
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
  float speed, maxSpeed=0, minSpeed=1000;   // minSpeed в МГц, поэтому ставим заведомо большое число

  GetNumberHSSpeeds(0, 0, &NumberOfSpeeds);
  for (int j=0; j < NumberOfSpeeds; j++){
    GetHSSpeed(0, 0, j, &speed);
    hss.push_back(speed);
    if (speed < minSpeed){
      min_hss_No = j;
      minSpeed = speed;
    }
    if (speed > maxSpeed){
      maxSpeed = speed;
      max_hss_No = j;
    }
  }

  min_vss_No = 0;
  minSpeed = 0;   // minSpeed в мкс, поэтому ставим заведомо маленькое значение
  maxSpeed = 10000;

  GetNumberVSSpeeds(&NumberOfSpeeds);
  for (int j=0; j < NumberOfSpeeds; j++){
    GetVSSpeed(j, &speed);
    vss.push_back(speed);
    if (speed > minSpeed){
      min_vss_No = j;
      minSpeed = speed;
    }
    if (speed < maxSpeed){
      max_vss_No = j;
      maxSpeed = speed;
    }
  }
}


string Camera::getModel(){
  return model;
}


std::string Camera::parseCommand(std::string message){
  vector<string> buffer;
  boost::split(buffer, message, boost::is_any_of(" \t\n\0"));
  string command = buffer.at(0);
  message.erase(0, command.length()+1);

  if (command.compare("IMAG") == 0){
    if (buffer.size() > 1) exposureTime = stof(buffer.at(1));
    log->print("Exposure time is set to %g", exposureTime);
    log->print("Getting image...");
    image();
    return string("Image acquired\n");
  }
  else if (command.compare("HEAD") == 0) {
    log->print("Editing header");
    try {
      std::string htvp = header.parseString(message);
      return string("OK ")+htvp+'\n';
    } catch(...) {
      log->print("ERROR Does value match type?");
      return string("ERROR STATUS=INVALID_ARGUMENT\n");
    }
  } 
  else if (command.compare("TEMP") == 0) {
    if (buffer.size() > 1) targetTemperature = stoi(message);
    log->print("Target temperature is set to %d", targetTemperature);
    setTemperature();
    return string("New target temperature is set\n");
  }
  else if (command.compare("SHTR") == 0) {
    if (buffer.size() > 1) shutterMode = stoi(message);
    setShutterMode();
    return string("Shutter mode is set\n");
  }
  else if (command.compare("PREF") == 0) {
    if (buffer.size() > 1) prefix = buffer.at(1);
    log->print("Prefix is set to %s", prefix.c_str());
    return string("Prefix is set\n");
  }
  else if (command.compare("SUFF") == 0) {
    if (buffer.size() > 1) postfix = buffer.at(1);
    log->print("Suffix is set to %s", postfix.c_str());
    return string("Suffix is set\n");
  }
  else if (command.compare("DIR") == 0) {
    if (buffer.size() > 1){
      writeDirectory = buffer.at(1);
      if (writeDirectory.back() != '/') writeDirectory += "/";
    }
    log->print("Target directory is set to %s", writeDirectory.c_str());
    return string("Target directory is set\n");
  }
  else if (command.compare("BIN") == 0) {
    if (buffer.size() > 2) bin(stoi(buffer.at(1)), stoi(buffer.at(2)));
    return string("Binning is set\n");
  }
  else if (command.compare("SPEED") == 0) {
    if (buffer.size() > 1) speed(buffer.at(1));
    return string("HSS speed is set\n");
  }
  else if (command.compare("VSPEED") == 0) {
    if (buffer.size() > 1) vspeed(buffer.at(1));
    return string("VSS speed is set\n");
  }
  else if (command.compare("GET") == 0) {
    updateStatement();
    return string("T =")+to_string(temperature)+"\n";
  }
  else if (command.compare("EXIT") == 0) {
    return string("Starting exit procedure\n");
  }
  else {
    log->print("Unknown command");
    return string("Unknown command\n");
  }
}


void Camera::image(){
  SetImage(hBin,vBin,crop.at(0),crop.at(1),crop.at(2),crop.at(3));
  SetExposureTime(exposureTime);
  string name = fileName();

  float exposure, accumulate, kinetic;
  GetAcquisitionTimings(&exposure, &accumulate, &kinetic);
  log->print("Exposure time is %g, accumulate is %g, kinetic is %g", exposure, accumulate, kinetic);
  log->print("Starting acquisition");
  StartAcquisition();

  //Loop until acquisition finished
  GetStatus(&status);
  while(status == DRV_ACQUIRING){
    GetStatus(&status);
    sleep(1);
  }
  if (status != DRV_IDLE){
    log->print("Error while acqiring data");
    exit(1);
  }
  log->print("Image is acquired");

  log->print("Saving file %s", name.c_str());

  status = SaveAsFITS((char *)name.c_str(), 0);   // Save as fits with ANDOR metadata
  if (status != DRV_SUCCESS){
    log->print("Error while saving fits");
//    exit(1);
  }
  log->print("Draft fits is saved.");
  header.update(name);  // Write additional header keys
  log->print("Result fits is saved.");
}


std::string Camera::fileName(){
  time_t cur_time = time(NULL);
  struct tm *curr_time = gmtime(&cur_time);
  char tfmt[50];
  string img_time;
  strftime(tfmt,50,"%Y%m%d%H%M%S",curr_time);
  img_time = tfmt;
  string result = writeDirectory + prefix + img_time + postfix + ".fits";
  log->print("File name: %s", result.c_str());
  return result;
}

void Camera::setTemperature(){
  if (targetTemperature < minT) {
    targetTemperature = minT;
    log->print("Target temperature is set to %d", targetTemperature);
  }
  else if (targetTemperature > maxT) {
    targetTemperature = maxT;
    log->print("Target temperature is set to %d", targetTemperature);
  }
  CoolerON();
  SetTemperature(targetTemperature);
}

void Camera::setShutterMode(){
  if (shutterMode > 4 || shutterMode < 0) shutterMode = 0;
  if (isInternalShutter){
    SetShutter(1,shutterMode,shutterOpenTime,shutterCloseTime);
    log->print("Internal shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  } else {
    SetShutterEx(1,shutterMode, shutterOpenTime, shutterCloseTime, shutterMode);
    log->print("External shutter is in '%s' mode.", shutterModes.at(shutterMode).c_str());
  }
}

void Camera::readIni(Config *ini){
  try {
    targetTemperature = int(ini->lookup("Temperature"));
  } catch(const SettingNotFoundException &nfex){
    targetTemperature = temperature;
  }

  try {
    string val = ini->lookup("Prefix");
    prefix = val;
  } catch(const SettingNotFoundException &nfex) {
    prefix = "";
  }

  try {
    string val = ini->lookup("Postfix");
    postfix = val;
  } catch(const SettingNotFoundException &nfex) {
    postfix = "";
  }

  try {
    string val = ini->lookup("Dir");
    writeDirectory = val;
  } catch(const SettingNotFoundException &nfex) {
    writeDirectory = "";
  }

  Setting &root = ini->getRoot();
  try {
    Setting &ini_header = root["Header"];
    int n = ini_header.getLength();
    for (int i = 0; i < n; ++i)
    {
      header.parseString(ini_header[i]);
    }
  } catch(const SettingNotFoundException &nfex) {
  }

}


void Camera::updateStatement(){
  log->print("Updating statement...");
  Setting &root = cfg.getRoot();

  root["TargetT"] = targetTemperature;
  root["Shutter"] = shutterModes.at(shutterMode);

  temperatureStatus = (GetTemperatureF(&temperature));
  root["Temperature"] = temperature;

  switch(temperatureStatus){
    case DRV_TEMP_STABILIZED:
      root["CoolingStatus"] = "Temperature has stabilized at set point";
      break;
    case DRV_TEMP_NOT_REACHED:
      root["CoolingStatus"] = "Temperature has not reached set point";
      break;
    case DRV_TEMP_DRIFT:
      root["CoolingStatus"] = "Temperature had stabilised but has since drifted";
      break;
    case DRV_TEMP_NOT_STABILIZED:
      root["CoolingStatus"] = "Temperature reached but not stabilized";
      break;
  }

  cfg.writeFile(configName.c_str());
}

void Camera::speed(string sp){
  if (sp.compare("MAX") == 0) hssNo = max_hss_No;
  else if (sp.compare("MIN") == 0) hssNo = min_hss_No;
  else {
    int N = stoi(sp);
    if (N >= 0 and N < hss.size()) hssNo = N;
  }
  SetHSSpeed(0, hssNo);
  log->print("Horizontal Shift Speed is set to %gMHz", hss.at(hssNo));
}

void Camera::vspeed(string sp){
  if (sp.compare("MAX") == 0) vssNo = max_vss_No;
  else if (sp.compare("MIN") == 0) vssNo = min_vss_No;
  else {
    int N = stoi(sp);
    if (N >= 0 and N < vss.size()) vssNo = N;
  }
  SetVSSpeed(vssNo);
  log->print("Vertical Shift Speed is set to %gus", vss.at(vssNo));
}

void Camera::endWork(){
  log->print("Command loop exit");
  targetTemperature = maxT;
  setTemperature();
  while(temperature < maxT - 1){
    GetTemperatureF(&temperature);
    updateStatement();
    log->print("Temperature %g", temperature);
    sleep(10);
  }
  CoolerOFF();
  ShutDown();
}


void Camera::bin(int hbin, int vbin) {
  hBin = hbin;
  vBin = vbin;
  log->print("Set bin: horizontal = %d, vertical = %d", hBin, vBin);
}
