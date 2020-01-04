#include "andor-daemon.h"

#define DEFAULT_PORT 1234

int CameraSelect (int iNumArgs, char* szArgList[]);
int temp(int T);
int Image(float t, char* prefix, fitsfile* file, FILE** log);
int Daemon(int argc, char* argv[]);
int Main(int argc, char* argv[]);
void LogFileInit(FILE** log, char* file_name);
void PrintInLog(FILE** log, char message[], ...);
void SocketInit(int* listener, struct sockaddr_in* addr, int port_number, FILE** log);
int GetMessage(int listener, char message[]);
void CameraInit(FILE** log);
void FitsInit(fitsfile** file);
int AddHeaderKey(char* message, fitsfile* file, FILE** log);
void UpdateHeader(fitsfile* file, char* file_name);
void FileName(char* prefix, char* message);
void Temperature(int T);
void UpdateStatement(config_t *cfg, FILE** log);
void Shutter(int mode, FILE** log);
void StatementInit(config_t* cfg, FILE** log);
void InitialSettings(config_t* ini, char* model_name, int* port, char** prefix, fitsfile* header, FILE** log);
int AddInitialKey(char* message, fitsfile* file, FILE** log);


int main(int argc, char* argv[]) {
  return Daemon(argc, argv);
}

int Daemon(int argc, char* argv[]) {
  pid_t process_id = 0;
  pid_t sid = 0;

  // Create child process
  process_id = fork();
  // Indication of fork() failure
  if (process_id < 0) {
    perror("fork failed!\n");
    // Return failure in exit status
    exit(1);
  }

  // PARENT PROCESS. Need to kill it.
  if (process_id > 0) {
    printf("process_id of child process %d \n", process_id);
    // return success in exit status
    exit(0);
  }

  /* THE NEXT PART OF CODE WILL BE RUN ONLY IN CHILD PROCESS */
  // Set new session
  sid = setsid();
  if (sid < 0) {
    perror("Can't set new session.\n");
    // Return failure
    exit(1);
  }

  // Main program
  Main(argc, argv);

  return (0);
}

int Main(int argc, char* argv[]){
  FILE *log = NULL;   // Log file

  config_t cfg;     // File to store camera statement
  config_t ini;     // Initial settings

  int port_number;
  int listener = 0;   // Socket descriptor
  struct sockaddr_in addr;  // Socket address
  char client_message[1024] = {0};  // Buffer for client message
  char *command;  // Buffer for command (separated from the message)
  char message[1024] = {0};
  char Model[40] = {0};
  char * prefix;

  fitsfile *template_fits;  // File to store header data
  int fits_status = 0;

  CameraInit(&log);     // Camera pre-setting and log creating

  GetHeadModel(Model);
  fits_create_file(&template_fits, "\!header.fits", &fits_status);   // File to store additional header informaion
  InitialSettings(&ini, Model, &port_number,&prefix, template_fits, &log);
  SocketInit(&listener, &addr, port_number, &log);    // Start TCP server

  StatementInit(&cfg, &log);    // Write down camera statement
  int maxT, minT, T;
  float Tf;

  do{
    int sock = GetMessage(listener, client_message);   // Recieve message from client
    PrintInLog(&log, client_message);   // Write this message in log
    strcpy(message, client_message);
    command = strtok(client_message, " \n\0");  // First word is command
    if (command == NULL) PrintInLog(&log, "Empty command");   // In case of...
    else {
      if (strcmp(command,"IMAG") == 0) fits_status = Image(atof(strtok(NULL, " \n\0")), prefix, template_fits, &log);
      else if (strcmp(command,"HEAD") == 0) fits_status = AddHeaderKey(message, template_fits, &log);
      else if (strcmp(command,"TEMP") == 0) Temperature(atoi(strtok(NULL, " \n\0")));
      else if (strcmp(command,"SHTR") == 0) Shutter(atoi(strtok(NULL, " \n\0")), &log);
      else if (strcmp(command,"GET") == 0) { 
        GetTemperatureF(&Tf); 
        sprintf(message,"Temperature %6.2f\n",Tf);
        PrintInLog(&log,message);
        send(sock, message , strlen(message) , 0 ); 
      }
      UpdateStatement(&cfg, &log);   // Write down camera statement
    }
    close(sock);    
  } while(strcmp(command,"EXIT") != 0);
   PrintInLog(&log, "Command loop exit");

  close(listener);
  fits_close_file(template_fits, &fits_status);
  GetTemperature(&T);
  GetTemperatureRange(&minT, &maxT);
  SetTemperature(maxT);
  while(T < maxT - 1){
    GetTemperature(&T);
    sleep(10);
  }
  CoolerOFF();
  time_t cur_time = time(NULL);
  fprintf(log, "\nLog ending %s\n", ctime(&cur_time));
  fclose(log);
  ShutDown();
  config_destroy(&cfg);

  return (0);
}

void LogFileInit(FILE** log, char* file_name){
  char buff[50] = {0};
  strcpy(buff, file_name);
  strcat(buff, ".log");
  printf("%s\n", buff);
  *log = fopen(buff, "a");
  // In case of errors
  if (*log == NULL) {
    perror("Can't open log file");
    // Return failure
    exit(1);
  }
  time_t curr_time = time(NULL);
  fprintf(*log, "\n\n\nLog beginning %s\n", ctime(&curr_time));
  fflush(*log);
}

void PrintInLog(FILE** log, char message[], ...){
  // Print string and time in log file
  time_t cur_time;
  struct tm *curr_time;
  char buffer[500];
  va_list arglist;

  GetHeadModel(buffer);
  fprintf(*log, "%s ", buffer);

  time(&cur_time);
  curr_time = localtime(&cur_time);

  strftime(buffer,500,"%T %h %d ",curr_time);
  fprintf(*log, "%s: ", buffer);

  char *msg;
  strcpy(buffer, message);
  msg = strtok(buffer, "\n\0");
  va_start(arglist, msg);
  vfprintf(*log, msg, arglist);
  va_end(arglist);

  fprintf(*log, "\n");
  fflush(*log);
}

void SocketInit(int* listener, struct sockaddr_in* addr, int port_number, FILE** log){
  PrintInLog(log, "Socket initialization...");
  *listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (*listener < 0){
    PrintInLog(log, "Can't create socket");
    exit(1);
  }
  PrintInLog(log, "Socket is created.");

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port_number);
  addr->sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(*listener, (struct sockaddr *)addr, sizeof(*addr)) < 0){
    PrintInLog(log, "Can't bind socket");
    exit(2);
  }
  PrintInLog(log, "Socket is bound on port %d.", port_number);

  listen(*listener, 1);
  PrintInLog(log, "TCP server is activated");
}

int GetMessage(int listener, char message[]){
  int sock = accept(listener, 0, 0);
    if(sock < 0){
        perror("Can't connect to the client\n");
        exit(3);
    }
  read(sock, message, 1024);
  return sock;
//  close(sock);
}

void CameraInit(FILE** log){
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
    printf("Can't find desired camera");
    exit(1);
  }

  char Model[40] = {0};
  GetHeadModel(Model);

  LogFileInit(log, Model);    // File to write all events

  PrintInLog(log, "Camera %s is initialized.", Model);

  //Set Read Mode to --Image--
  SetReadMode(4);
  PrintInLog(log, "Read mode is set to 'Image'.");

  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode(1);
  PrintInLog(log, "Acquisition mode is set to 'Single Scan'.");

  //Set initial exposure time
  SetExposureTime(0.1);
  PrintInLog(log, "Exposure time is 0.1.");

  //Initialize Shutter
  int InternalShutter = 0;  // This flag shows if camera has an internal shutter
  IsInternalMechanicalShutter(&InternalShutter); // Checking existance of internal shutter
  if (InternalShutter){
    // TTL to open is high (1), mode is fully-auto, time to open and close is about 50ms
    SetShutter(1,0,50,50);
  } else {
    // TTL to open is high (1), mode is fully-auto,
    // time to open and close is about 50ms, external shutter mode is fully-auto
    SetShutterEx(1, 0, 50, 50, 0);
  }
  PrintInLog(log, "Shutter is in Auto mode.");

  int NumberOfSpeeds, minSpeedIndex=0;
  float speed, minSpeed=100;

  GetNumberHSSpeeds(0, 0, &NumberOfSpeeds);
  for (int j=0; j<NumberOfSpeeds; j++){
    GetHSSpeed(0, 0, j, &speed);
    if (speed < minSpeed){
      minSpeedIndex = j;
      minSpeed = speed;
    }
  }
  SetHSSpeed(0, minSpeedIndex);
  PrintInLog(log, "Horizontal Shift Speed is set to %gMHz", minSpeed);

  minSpeedIndex = 0;
  minSpeed = 0;

  GetNumberVSSpeeds(&NumberOfSpeeds);
  for (int j=0; j<NumberOfSpeeds; j++){
    GetVSSpeed(j, &speed);
    if (speed > minSpeed){
      minSpeedIndex = j;
      minSpeed = speed;
    }
  }
  SetVSSpeed(minSpeedIndex);
  PrintInLog(log, "Vertical Shift Speed is set to %gus", minSpeed);
}

void FitsInit(fitsfile** file){
  int status = 0;
  fits_create_file(file, "\!header.fits", &status);
  //fits_create_img(*file, LONG_IMG, 2, naxis, &status);
}


int Image(float t, char* prefix, fitsfile* file, FILE** log){
  int width, height;
  GetDetector(&width, &height);
  SetImage(1,1,1,width,1,height); //horiz bin, vert bin, first xpix, last xpix, first ypix, last ypix
  char* file_name;
  FileName(prefix, file_name);

  SetExposureTime(t);
  float exposure, accumulate, kinetic;
  GetAcquisitionTimings(&exposure, &accumulate, &kinetic);
  PrintInLog(log, "Exposure time is %g, accumulate is %g, kinetic is %g", exposure, accumulate, kinetic);
  PrintInLog(log, "Starting acquisition");
  StartAcquisition();
  int status = 0;

  //Loop until acquisition finished
  GetStatus(&status);
  while(status == DRV_ACQUIRING){
    GetStatus(&status);
  }
  if (status != DRV_IDLE){
    PrintInLog(log, "Error while acqiring data");
    return 0;
  }
  PrintInLog(log, "Image is acquired");

  PrintInLog(log, "Saving file %s", file_name);
  //printf("%s\n", file_name);

  status = SaveAsFITS(file_name, 2);   // Save as fits with ANDOR metadata
  if (status != DRV_SUCCESS){
    PrintInLog(log, "Error while saving fits");
    return 0;
  }
  PrintInLog(log, "Draft fits is saved.");
  UpdateHeader(file, file_name);  // Write additional header keys
  PrintInLog(log, "Result fits is saved.");

  return 0;
}

int AddHeaderKey(char* message, fitsfile* file, FILE** log){
  message = strtok(message, "\n");
  strtok(message, " \0\n");
  char* key = strtok(NULL, " \0\n");
  char* value = strtok(NULL, "\\\0\n");
  char* comment = strtok(NULL, "\\\0\n");
  int status = 0;
  fits_update_key(file, TSTRING, key, value, comment, &status);
  return 0;
}

int AddInitialKey(char* message, fitsfile* file, FILE** log){
  char* key = strtok(message, " \0\n");
  char* value = strtok(NULL, "\\\0\n");
  char* comment = strtok(NULL, "\\\0\n");
  int status = 0;
  fits_update_key(file, TSTRING, key, value, comment, &status);
  return 0;
}

void FileName(char* prefix, char* message){
  time_t cur_time = time(NULL);
  struct tm *curr_time = gmtime(&cur_time);
  char tfmt[50];
  sprintf(tfmt,"./data/%s%%Y%%m%%d%%H%%M%%S.fits",prefix);
  strftime(message,50,tfmt,curr_time);
//  strftime(message,50,"TDS%Y%m%d%H%M%S.fits",curr_time);
}

void UpdateHeader(fitsfile* file, char* file_name){
  int status = 0;
  int n = 0;
  fitsfile* image;
  char card[100]={0};
  fits_open_data(&image, file_name, READWRITE, &status);
  fits_get_hdrspace(file, &n, NULL, &status);
  for (int i=1; i < n+1; i++){
    fits_read_record(file, i, card, &status);
    fits_write_record(image, card, &status);
  }
  fits_close_file(image, &status);
}

void Temperature(int T){
  CoolerON();
  SetTemperature(T);
}

void UpdateStatement(config_t* cfg, FILE** log){
  PrintInLog(log, "Updating statement...");

  config_setting_t *root, *setting;
  char str[1024] = {0};
  float value = 0;

  root = config_root_setting(cfg);

  GetTemperatureF(&value);
  setting = config_setting_get_member(root, "Temperature");
  config_setting_set_float(setting, value);
  PrintInLog(log, "Temperature %g", value);

  GetHeadModel(str);
  static const char *output_file = strcat(str, ".info");

  /* Write out the new configuration. */
  if(! config_write_file(cfg, output_file)){
    PrintInLog(log, "Can't update configuration!");
  } else {
    PrintInLog(log, "Configuration was updated.");
  }
}

void Shutter(int mode, FILE** log){
  if (mode > 2) {
    PrintInLog(log, "Incorrect shutter mode!");
    return;
  }
  int InternalShutter = 0;  // This flag shows if camera has an internal shutter
  IsInternalMechanicalShutter(&InternalShutter); // Checking existance of internal shutter
  if (InternalShutter){
    // TTL to open is high (1), mode is fully-auto, time to open and close is about 50ms
    SetShutter(1, mode, 50, 50);
  } else {
    // TTL to open is high (1), mode is fully-auto,
    // time to open and close is about 50ms, external shutter mode is fully-auto
    SetShutterEx(1, mode, 50, 50, mode);
  }

  switch (mode){
    case 0:
      PrintInLog(log, "Shutter is in Auto mode.");
      break;
    case 1:
      PrintInLog(log, "Shutter is opened.");
      break;
    case 2:
      PrintInLog(log, "Shutter is closed.");
  }
}

void StatementInit(config_t* cfg, FILE** log){
  config_init(cfg);
  PrintInLog(log, "Updating statement...");

  config_setting_t *root, *setting;
  int number;
  char str[1024] = {0};
  float value = 0;

  root = config_root_setting(cfg);

  number = getpid();
  setting = config_setting_add(root, "Daemon", CONFIG_TYPE_INT);
  config_setting_set_int(setting, number);
  PrintInLog(log, "PID %d", number);

  GetCameraSerialNumber(&number);
  setting = config_setting_add(root, "Serial", CONFIG_TYPE_INT);
  config_setting_set_int(setting, number);
  PrintInLog(log, "Serial %d", number);

  GetHeadModel(str);
  setting = config_setting_add(root, "Model", CONFIG_TYPE_STRING);
  config_setting_set_string(setting, str);
  PrintInLog(log, "Model %s", str);
  static const char *output_file = strcat(str, ".info");

  GetTemperatureF(&value);
  setting = config_setting_add(root, "Temperature", CONFIG_TYPE_FLOAT);
  config_setting_set_float(setting, value);
  PrintInLog(log, "Temperature %.2f", value);



  /* Write out the new configuration. */
  if(! config_write_file(cfg, output_file)){
    PrintInLog(log, "Can't update configuration!");
  } else {
    PrintInLog(log, "Configuration was updated.");
  }
}

void InitialSettings(config_t* ini, char* model_name, int* port, char** prefix, fitsfile* file_h, FILE** log){
  config_setting_t *root, *header;

  config_init(ini);
  char file_name[40] = {0};
  strcpy(file_name, model_name);
  strcat(file_name, ".ini");
  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(ini, file_name))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(ini),
            config_error_line(ini), config_error_text(ini));
    config_destroy(ini);
    exit(EXIT_FAILURE);
  }
  root = config_root_setting(ini);
  config_setting_lookup_int(root, "Port", port);
  config_setting_lookup_string(root, "Prefix", (const char**)prefix);
  header = config_lookup(ini, "Header");
  if(header != NULL)
  {
    int count = config_setting_length(header);
    int i;

    PrintInLog(log, "Initial header values:");
    for(i = 0; i < count; ++i)
    {
      config_setting_t *header_key = config_setting_get_elem(header, i);
      const char * str = config_setting_get_string(header_key);
      char str2[100] = {0};
      strcpy(str2, str);
      PrintInLog(log, str2);
      AddInitialKey(str2, file_h, log);
    }
  }
}
