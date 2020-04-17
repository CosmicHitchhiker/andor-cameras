#include "camera.h"

using namespace std;
using namespace libconfig;


///  Конструктор класса: задаёт начальные параметры, не зависящие от файла конфигурации.
/**
  __ВАЖНО:__ при создании класс ничего не знает про лог-файл, поэтому в конструкторе
  непосредственные параметры реальной камеры __не меняются__

  Заполняет переменные #readModes, #acquisitionModes, #shutterModes и #shutterMStatus.

  Устанавливает следующие значения переменных по-умолчанию:
  - #readMode = 4 (Image)
  - #acquisitionMode = 0 (Single Scan)
  - #exposureTime = 0.1 секунда
  - #shutterMode = 0 (Fully Auto)
  - #shutterOpenTime и #shutterCloseTime по 50мс каждый
  - #hBin и #vBin = 1 (отсутствие бинирования)
  - #expstarted = false
  - #prefix, #postfix и #writeDirectory зануляются
  - #dataType = 0 (unsigned 16)

  Также настраивает файл .info, в который будут записываться
  целевая температура, текущая температура, статус охлаждения
  и режим затвора.

  В том случае, если работа ведётся с реальной камерой (`isParent == True`),
  то дополнительно выполняются следующие действия:
  - Производится инициализация камеры методом andorInit()
  - В переменную #model записывается название модели подключённой камеры
  - Создаётся файл для записи текущего состояния камеры
  - Проверяется наличие внутреннего затвора
  - Определяются ширина и высота камеры (с записью в #width и #height)
  - #crop устанавливается на весь размер камеры
  - Получаются возможные скорости считывания и #hssNo, #vssNo присуждаются значения минимальных
  - Включается охладитель и получается возможный диапазон температур (#minT, #maxT)
  - Целевой температурой охлаждения назначается текущая температура (охлаждения не происходит)
*/
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
  shutterMStatus = {"AUTO", "OPEN", "CLOSED", "OPEN_FOR_FVB", "OPEN_FOR_SERIES"};
  readMode = 4;
  acquisitionMode = 0;
  exposureTime = 0.1;
  shutterMode = 0;
  shutterOpenTime = 50;
  shutterCloseTime = 50;
  hBin = 1;
  vBin = 1;
  dataType = 0;
  expstarted = false;

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


/// Предварительная настройка камеры в соответствии с файлом инициализации.
/**
  Этот метод обо всех своих действиях сообщает в лог-файл!

  Производится настройка камеры в соответствии со значениями по-умолчанию:
  режим считывания, режим накопления, режим затвора, скорости считывания.

  Производится настройка камеры командой readIni() в соответствии с .ini файлом.

  Обновляется текущее состояние камеры (updateStatement())

  @param logFile указатель на лог-файл (класс Log)
  @param ini указатель на файл с предварительными настройками
  (класс __Config__ библиотеки _libconfig_)
*/
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

  // Чтение и установка предварительных настроек
  readIni(ini);
  // Установка целевой температуры, указанной в предварительных настройках
  setTemperature();
  // Запись текущего состояния в файл
  updateStatement();
}


/// Инициализация камеры библиотекой AndorSDK
/**
  Метод получает общее количество камер Андор, поключённых к компьютеру,
  затем поочереди пытается к ним подключиться.
  После того, как к какой-то камере удалось подключиться - она инициализируется.
  Если ни к одной камере подключиться не удалось - веполнение __всей__ программы прерывается.
*/
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


/// Получение информации о возможных режимах считывания (скоростях сдвига)
/**
  Метод используя процедуры библиотеки AndorSDK заполняет переменные
  #hss, #vss, #min_hss_No, #max_hss_No, #min_vss_No, #max_vss_No
 */
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


/// Возвращает модель камеры (#model)
string Camera::getModel(){
  return model;
}


/// Основная функция для взаимодействия с камерой
/**
  Метод получает на вход сообщение, которое затем обрабатывает как команду.

  __ВАЖНО__: этот метод общий для реальной и виртуальной камер. Поэтому он
  не должен содержать прямого обращения к камере, а лишь вызывать другие
  методы этого класса.

  Первая часть обработки - разделение сообщения на слова. Разделительными
  символами могут быть пробел, табуляция и символы конца строки.

  Первое слово строки считается командой. Дальнейшее поведение программы
  зависит от того, какая это команда. В случе, если кроме самой команды
  сообщение содержит какие-либо параметры, эти параметры проверяются на
  соответствие типа, если тип некорректен, метод возвращает сообщение
  об ошибке.

  ####Возможное поведение метода.

  Команда __IMAG__:
  Запускает экспозицию.
  Команда может содержать дополнительный параметр - время экспозиции в секундах.
  В случае неверного типа параметра - возвращает сообщение ошибки.
  При отсутствии этого параметра время экспозиции остаётся таким же, как ранее.
  Метод возвращает результат выполнения startExposure()

  Команда __ABORT__:
  В случае, если идёт съёмка, прерывает её и возвращает сообщение со статусом.
  Если съёмка не идёт - возвращает сообщение об ошибке со статусом.

  Команда __HEAD__:
  Передаёт всё сообщение, кроме самой команды, методу HeaderValues::parseString()
  В случае ошибки (например, несоответствие типа значения и переданного значения)
  возвращает сообщение об ошибке.
  Если всё успешно - возращает результат выполнения HeaderValues::parseString()

  Команда __TEMP__:
  Устанавливает целевую температуру.
  Может иметь дополнительный аргумент - целочисленное значение целевой температуры.
  Если переданный аргумент не преобразуется в целочисленный - вернёт сообщение об ошибке.
  В ином случае записывает в #targetTemperature переданное значение, вызывает метод setTemperature()
  и возвращает сообщение со значением целевой температуры и статусом охлаждения.

  Команда __SHTR__:
  Устанавливает режим работы затвора.
  Дополнительный аргумент - номер режима (см. AndorSDK)
  В случае наличия аргумента вызывает setShutterMode()
  Если аргумент некорректен - вернёт сообщение об ошибке.
  В ином случае возвращает статус затвора.

  Команда __PREF__:
  Устанавливает префикс в именах сохраняемых в дальнейшем fits-файлов (#prefix)
  Дополнительный аргумент - значение префикса.
  __КОМАНДА НЕ ПРОВЕРЯЕТ ОТСУТСТВИЕ ЗАПРЕЩЁННЫХ СИМВОЛОВ__
  Если значение префикса равно "*" - обнуляет префикс.
  Возвращает текущее значение префикса.

  Команда __SUFF__:
  Работает так же, как __PREF__, но изменяет суффикс (#postfix)

  Команда __DIR__:
  Работает так же как __PREF__, но изменяет папку сохранения (#writeDirectory)
  Если в конце аргумента не стоит слэш - добавляет его.

  Команда __BIN__:
  Задаёт бинирование при съёмке.
  Дополнительные аргументы - значения горизонтального и вертикального бинирования.
  Если аргуметны заданы - вызывает метод bin()
  В случае некорректных значений аргументов - возвращает сообщение об ошибке.
  Если всё в порядке - возвращает значения установленного горизонтального и вертикального бинирования.

  Команда __SPEED__:
  Устанавливает скорость считывания (скорость горизонтального сдвига)
  Опциональный аргумент - какую скорость установить (MIN, MAX или номер)
  Вызывает метод speed()
  Возвращает установленный номер и значение HSS.

  Команда __VSPEED__:
  Работает так же, как __SPEED__, но обрабатывает VSS (вызывая vspeed())

  Команда __GET__:
  Получение информации о камере.
  Дополнительный аргумент - уточнение какую информацию получить.
  STATUS - статус камеры
  TCCD - температура матрицы
  TIME - оставшееся время экспозиции (0, если экспозиция не идёт)
  Возвращает соответствующий запросу параметр.

  Команда __EXIT__:
  Возвращает информацию о том, что началась процедура выхода.

  __Любая другая команда__:
  Возвращает сообщение о некорректной команде.

  @param message Строка, содержащая сообщение клиента.
  @return Строка, результат выполнения методов класса Camera или сообщение об ошибке.
*/
std::string Camera::parseCommand(std::string message){
  vector<string> buffer;
  boost::split(buffer, message, boost::is_any_of(" \t\n\0"));
  string command = buffer.at(0);
  message.erase(0, command.length()+1);

  if (command.compare("IMAG") == 0){
    if (buffer.size() > 1) try {
      exposureTime = stof(buffer.at(1));
    } catch(...) {
      log->print("ERROR Invalid argument ",buffer.at(1)," must be float");
      return string("ERROR STATUS=INVALID_ARGUMENT\n");
    }
    log->print("Getting image with exposure time %g", exposureTime);
    return startExposure();
  }
  else if (command.compare("ABORT") == 0) {
    if (status==DRV_ACQUIRING) {
      log->print("Aborting exposure");
      AbortAcquisition();
      GetStatus(&status);
      // AbortAcquisition does not retain accumulated signal, so there is no reason to readout (??):
      expstarted = 0;
      return string("OK STATUS=")+textStatus(status)+'\n';
    } else {
      return string("ERROR STATUS=")+textStatus(status)+'\n';
    }
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
    if (buffer.size() > 1) try {
      targetTemperature = stoi(buffer.at(1));
      log->print("Target temperature is set to %d", targetTemperature);
    } catch(...) {
      log->print("ERROR Invalid argument %s should be integer", buffer.at(1).c_str());
      return string("ERROR STATUS=INVALID_ARGUMENT\n");
    }
    setTemperature();
    return string("OK TARGET_TEMP=")+to_string(targetTemperature)+" TEMP_STATUS="+to_string(temperatureStatus)+'\n';
  }

  else if (command.compare("SHTR") == 0) {
    if (buffer.size() > 1) try {
      shutterMode = stoi(buffer.at(1));
      setShutterMode();
    } catch(...) {
      log->print("ERROR Invalid argument %s must be integer", buffer.at(1).c_str());
      return string("ERROR STATUS=INVALID_ARGUMENT\n");
    }
    return string("OK SHTR=")+to_string(shutterMode) + \
           string("SHTR_TXT=") + shutterMStatus.at(shutterMode)+'\n';
  }

  // TODO: Сделать контроль запрещённых символов (*?/)
  else if (command.compare("PREF") == 0) {
    if (buffer.size() > 1){
      prefix = buffer.at(1);
      if (prefix.compare("*") == 0) {
        prefix = "";
        log->print("Reset prefix");
      } else
        log->print("Prefix is set to %s", prefix.c_str());
    }
    return string("OK PREF=")+prefix+'\n';
  }

  else if (command.compare("SUFF") == 0) {
    if (buffer.size() > 1) {
      postfix = buffer.at(1);
      if (postfix.compare("*") == 0){
        postfix = "";
        log->print("Reset suffix");
      } else
        log->print("Suffix is set to %s", postfix.c_str());
    }
    return string("OK SUFF=")+postfix+'\n';
  }
  else if (command.compare("DIR") == 0) {
    if (buffer.size() > 1){
      writeDirectory = buffer.at(1);
      if (writeDirectory.compare("*")){
        writeDirectory = "";
        log->print("Target directory is reset");
      } else {
        if (writeDirectory.back() != '/') writeDirectory += "/";
        log->print("Target directory is set to %s", writeDirectory.c_str());
      }
    }
    return string("OK DIR=")+writeDirectory+'\n';
  }

  else if (command.compare("BIN") == 0) {
    if (buffer.size() > 2) try {
      return bin(stoi(buffer.at(1)), stoi(buffer.at(2)));
    } catch(...) {
      log->print("ERROR Invalid arguments %s and %s must be integers", buffer.at(1).c_str(), buffer.at(2).c_str());
      return string("ERROR STATUS=INVALID_ARGUMENT\n");
    }
  }
  else if (command.compare("SPEED") == 0) {
    if (buffer.size() > 1) speed(buffer.at(1));    
    return string("OK SPEED=")+to_string(hssNo)+" HSS="+to_string(hss.at(hssNo))+'\n';
  }
  else if (command.compare("VSPEED") == 0) {
    if (buffer.size() > 1) vspeed(buffer.at(1));
    return string("OK VSPEED=")+to_string(vssNo)+" VSS="+to_string(vss.at(vssNo))+'\n';
  }
  else if (command.compare("GET") == 0) {
    std::string reply = "OK";
    if (std::find(buffer.begin(),buffer.end(),"STATUS")!=buffer.end()) {
      GetStatus(&status);
      reply += string(" STATUS=")+textStatus(status);
    }
    if (std::find(buffer.begin(),buffer.end(),"TCCD")!=buffer.end()) {
      updateStatement();
      reply += string(" TCCD=")+to_string(temperature);
    }
    if (std::find(buffer.begin(),buffer.end(),"TIME")!=buffer.end()) {
      int timeleft = 0;
      if (expstarted) {
        time_t currTime;
        time(&currTime);
        timeleft = exposureTime - difftime(currTime, startTime);
        if (timeleft<0) timeleft = 0;
      }
      reply += string(" TIME=")+to_string(timeleft);
    }
    return reply+'\n';
  }
  else if (command.compare("EXIT") == 0) {
    return string("OK STATUS=EXIT_PROCEDURE_STARTED\n");
  }
  else {
    log->print("Unknown command");
    return string("ERROR STATUS=BAD_COMMAND\n");
  }
}


/// Настройка и запуск экспозиции
/**
  Задаются параметры кадра (#hBin, #vBinm #crop передаются SetImage() AndorSDK)
  Время экспозиции устанавливается передачей #exposureTime в SetExposureTime() AndorSDK.
  Генерируется имя файла функцией fileName()

  В лог-файл передаются значения времён, полученные GetAquisitionTimings() AndorSDK.
  Запускается съёмка StartAquisition() и время начала записывается в #startTime.
  Если экспозиция началасть, #expstarted становится "true" и в #status записывается текущий статус.

  @returns Возвращает сообщение с именем файла, который будет записан и статусом или сообщение с ошибкой.
*/
std::string Camera::startExposure(){
  SetImage(hBin,vBin,crop.at(0),crop.at(1),crop.at(2),crop.at(3));
  SetExposureTime(exposureTime);
  fname = fileName();

  float exposure, accumulate, kinetic;
  GetAcquisitionTimings(&exposure, &accumulate, &kinetic);
  log->print("Exposure time is %g, accumulate is %g, kinetic is %g", exposure, accumulate, kinetic);
  log->print("Starting acquisition");
  status = StartAcquisition();
  time(&startTime);

  if (status == DRV_SUCCESS) {
    expstarted = true;
    GetStatus(&status);
    return std::string("OK FILE=") + fname + " EXPTIME="+to_string(exposure)+" STATUS="+textStatus(status)+'\n';
  } else {
    return std::string("ERROR STATUS=")+textStatus(status)+'\n';
  }
}


/// Возвращает информацию о том, есть ли несохранённый кадр в камере
/**
  Возвращает flase, если экспозиция ещё не была начата.
  */
bool Camera::imageReady() { 
  if (!expstarted) return false;
  GetStatus(&status);
  return status!=DRV_ACQUIRING;
}


/// Переводит численное значение статуса в текстовое
std::string Camera::textStatus(int status_) {
  switch (status_) {
    // All:
    case DRV_SUCCESS: return "SUCCESS";
    // GetStatus:
    case DRV_IDLE : return "IDLE";
    case DRV_ACQUIRING : return "ACQUIRING";
    case DRV_TEMPCYCLE : return "TEMPCYCLE";
    case DRV_ACCUM_TIME_NOT_MET : return "ACCUM_TIME_NOT_MET";
    case DRV_KINETIC_TIME_NOT_MET: return "KINETIC_TIME_NOT_MET";
    case DRV_ERROR_ACK : return "ERROR_ACK";
    case DRV_ACQ_BUFFER: return "ACQ_BUFFER";
    case DRV_SPOOLERROR: return "SPOOLERROR";
    // StartAcquisition:
    case DRV_NOT_INITIALIZED: return "NOT_INITIALIZED";
    case DRV_VXDNOTINSTALLED: return "VXDNOTINSTALLED";
    case DRV_INIERROR: return "INIERROR";
    case DRV_ACQUISITION_ERRORS: return "ACQUISITION_ERRORS";
    case DRV_ERROR_PAGELOCK: return "ERROR_PAGELOCK";
    case DRV_INVALID_FILTER : return "INVALID_FILTER";
    case DRV_BINNING_ERROR : return "BINNING_ERROR";
    //SaveAsFits:
    case DRV_P1INVALID: return "P1INVALID";
    case DRV_P2INVALID: return "P2INVALID";
    case DRV_NOT_SUPPORTED: return "NOT_SUPPORTED";
    // GetTemperature:
    case DRV_TEMP_STABILIZED: return "TEMP_STABILIZED";
    case DRV_TEMP_NOT_REACHED: return "TEMP_NOT_REACHED";
    case DRV_TEMP_NOT_STABILIZED: return "TEMP_NOT_STABILIZED";
    case DRV_TEMP_OFF : return "TEMP_OFF";
    case DRV_TEMP_DRIFT : return "TEMP_DRIFT";
    default: return "UNKNOWN_STATUS_"+to_string(status);
  }
}


/// Сохраняет последнее записанное изображение
std::string Camera::saveImage() {
  //Loop until acquisition finished
  if (!expstarted) return std::string("ERROR STATUS=EXPOSURE_NOT_STARTED\n");
  GetStatus(&status);
  if (status == DRV_ACQUIRING) return std::string("ERROR STATUS=EXPOSURE_NOT_FINISHED\n");
  if (status != DRV_IDLE){
    log->print("Error while acquiring data");    
    return std::string("ERROR STATUS=")+textStatus(status)+'\n';
  }
  expstarted = false;
  log->print("Saving acquired image %s", fname.c_str());

  status = SaveAsFITS((char *)fname.c_str(), dataType);   // Save as fits with ANDOR metadata
  if (status != DRV_SUCCESS){
    log->print("Error while saving fits");
    std::string backup = "BACKUP.fits";
    if (SaveAsFITS((char *)backup.c_str(), 0)==DRV_SUCCESS) {
      log->print("ATTENTION! Backup file ",backup.c_str()," is (over)written!");
      return std::string("ERROR FILE=")+backup+" STATUS="+textStatus(status)+'\n';
    }
    return std::string("ERROR STATUS=")+textStatus(status)+'\n';
//    exit(1);
  }
  log->print("Draft fits is saved.");
  header.update(fname);  // Write additional header keys
  log->print("Result fits is saved.");
  return std::string("OK STATUS=IDLE\n");
}


/// Формирует имя файла для сохранения
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


/// Устанавливает целевую температуру детектора
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
  temperatureStatus = (GetTemperatureF(&temperature));
}


/// Устанавливает режим работы затвора
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


/// Считывает конфиг-файл и устанавливает заданные в нём значения переменных
void Camera::readIni(Config *ini){
  try {
    targetTemperature = int(ini->lookup("Temperature"));
  } catch(const SettingNotFoundException &nfex){
    targetTemperature = temperature;
  }

  try {
    string datatype = ini->lookup("Type");
    if (datatype.compare("uint16") == 0) dataType = 0;
    else if (datatype.compare("uint32") == 0) dataType = 1;
    else if (datatype.compare("int16") == 0) dataType = 2;
    else if (datatype.compare("int32") == 0) dataType = 3;
    else if (datatype.compare("float") == 0) dataType = 4;
    else dataType = 0;
  } catch(const SettingNotFoundException &nfex){
    dataType = 0;
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


/// Обновляет информацию о текущем состоянии камеры
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


/// Устанавливает скорость считывания
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


/// Устанавливает скорость вертикального сдвига
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


/// Завершает работу камеры
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


/// Задаёт бинирование
std::string Camera::bin(int hbin, int vbin) {
  if ((!(hbin == 0) && !(hbin & (hbin - 1))) && (!(vbin == 0) && !(vbin & (vbin - 1))) \
  	&& hbin <= width && vbin <= height){
	  hBin = hbin;
	  vBin = vbin;
	  log->print("Set bin: horizontal = %d, vertical = %d", hBin, vBin);
	  return string("OK HBIN=")+to_string(hBin)+" VBIN="+to_string(vBin)+'\n';
	} else {
	  log->print("ERROR Incorrect bin values");
	  return string("ERROR STATUS=INVALID_ARGUMENT\n");
	}
}
