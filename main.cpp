// TODO: make an extension for empty commands
// TODO: Make temperature in camera.info shorter
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "main.h"

#define DEFAULT_PORT 1234

int CameraSelect (int iNumArgs, char* szArgList[]);
int temp(int T);
int Image(float t, fitsfile* file, FILE** log);
int Daemon(int argc, char* argv[]);
int Main(int argc, char* argv[]);
void LogFileInit(FILE** log);
void PrintInLog(FILE** log, char message[]);
void SocketInit(int* listener, struct sockaddr_in* addr, int port_number, FILE** log);
void GetMessage(int listener, char message[]);
void CameraInit(FILE** log);
void FitsInit(fitsfile** file);
void mGetDetector(int *x, int *y);
int AddHeaderKey(char* message, fitsfile* file, FILE** log);
void UpdateHeader(fitsfile* file, char* file_name);
void FileName(char* message);
void Temperature(int T);
void UpdateStatement(config_t *cfg, FILE** log);
void Shutter(int mode, FILE** log);
void StatementInit(config_t* cfg, FILE** log);






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

  int listener = 0;   // Socket descriptor
  struct sockaddr_in addr;  // Socket address
  int port_number = DEFAULT_PORT;   // TCP port number
  if (argc > 1)
    port_number = atoi(argv[1]);
  char client_message[1024] = {0};  // Buffer for client message
  char *command;  // Buffer for command (separated from the message)
  char message[1024] = {0};

  fitsfile *template_fits;  // File to store header data
  int fits_status = 0;

  LogFileInit(&log);    // File to write all events

  SocketInit(&listener, &addr, port_number, &log);    // Start TCP server

  CameraInit(&log);     // Camera pre-setting

  fits_create_file(&template_fits, "\!header.fits", &fits_status);   // File to store additional header informaion

  StatementInit(&cfg, &log);    // Write down camera statement

  do{
    GetMessage(listener, client_message);   // Recieve message from client
    PrintInLog(&log, client_message);   // Write this message in log
    strcpy(message, client_message);
    command = strtok(client_message, " \n\0");  // First word is command
    if (command == NULL) PrintInLog(&log, "Empty command");   // In case of...
    else if (strcmp(command,"image") == 0) fits_status = Image(atof(strtok(NULL, " \n\0")), template_fits, &log);
    else if (strcmp(command,"header") == 0) fits_status = AddHeaderKey(message, template_fits, &log);
    else if (strcmp(command,"temp") == 0) Temperature(atoi(strtok(NULL, " \n\0")));
    else if (strcmp(command,"shutt") == 0) Shutter(atoi(strtok(NULL, " \n\0")), &log);
    UpdateStatement(&cfg, &log);   // Write down camera statement
  } while(strcmp(command,"exit") != 0);

  
  close(listener);
  fits_close_file(template_fits, &fits_status);
  int maxT, minT, T;
  GetTemperature(&T);
  GetTemperatureRange(&minT, &maxT);
  SetTemperature(maxT);
  while(T < maxT - 1){
    GetTemperature(&T);
  }
  CoolerOFF();
  time_t cur_time = time(NULL);
  fprintf(log, "\nLog ending %s\n", ctime(&cur_time));
  fclose(log);
  ShutDown();
  config_destroy(&cfg);

  return (0);
}

void LogFileInit(FILE** log){
  *log = fopen("log.txt", "a");
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

void PrintInLog(FILE** log, char message[]){
  // Print string and time in log file
  time_t cur_time;
  struct tm *curr_time;
  char buffer[500];

  time(&cur_time);
  curr_time = localtime(&cur_time);

  strftime(buffer,500,"%T %h %d ",curr_time);
  fprintf(*log, "%s: ", buffer);
  char *msg;
  strcpy(buffer, message);
  msg = strtok(buffer, "\n\0");
  fprintf(*log, "%s\n", msg);
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
  PrintInLog(log, "Socket is bound.");

  listen(*listener, 1);
  PrintInLog(log, "TCP server is activated");
}

void GetMessage(int listener, char message[]){
  int sock = accept(listener, 0, 0);
    if(sock < 0){
        perror("Can't connect to the client\n");
        exit(3);
    }
  read(sock, message, 1024);
  close(sock);
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
    PrintInLog(log, "Initialisation error...exiting");
    exit(1);
  }

  sleep(2); //sleep to allow initialization to complete
  PrintInLog(log, "Camera is initialized.");

  //Set Read Mode to --Image--
  SetReadMode(4);
  PrintInLog(log, "Read mode is set to 'Image'.");

  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode(1);
  PrintInLog(log, "Acquisition mode is set to 'Single Scan'.");

  //Set initial exposure time
  SetExposureTime(0.1);
  PrintInLog(log, "Exposure time is 0.1.");

  //SetNumberKinetics(1);

  //SetKineticCycleTime(2);

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
}

void FitsInit(fitsfile** file){
  int status = 0;
  fits_create_file(file, "\!header.fits", &status);
  //fits_create_img(*file, LONG_IMG, 2, naxis, &status);
}


int Image(float t, fitsfile* file, FILE** log){
  int width, height;
  GetDetector(&width, &height);
  SetImage(1,1,1,width,1,height); //horiz bin, vert bin, first xpix, last xpix, first ypix, last ypix
  char* file_name;
  FileName(file_name);

  SetExposureTime(t);
  StartAcquisition();
  int status;

  //Loop until acquisition finished
  GetStatus(&status);
  while(status==DRV_ACQUIRING) GetStatus(&status);
  PrintInLog(log, "Image is acquired");

  SaveAsFITS(file_name, 2);   // Save as fits with ANDOR metadata
  PrintInLog(log, "Draft fits is saved.");
  UpdateHeader(file, file_name);  // Write additional header keys
  PrintInLog(log, "Result fits is saved.");

  return 0;
}

int AddHeaderKey(char* message, fitsfile* file, FILE** log){
  message = strtok(message, "\n");
  char* command = strtok(message, " \0\n");
  char* key = strtok(NULL, " \0\n");
  char* value = strtok(NULL, " \0\n");
  char* comment = strtok(NULL, " \0\n");
  int status = 0;
  fits_update_key(file, TSTRING, key, value, comment, &status);
  return 0;
}

void FileName(char* message){
  time_t cur_time = time(NULL);
  struct tm *curr_time = gmtime(&cur_time);

  strftime(message,50,"KGO%Y%m%d%H%M%S.fits",curr_time);
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

  static const char *output_file = "camera.info";
  config_setting_t *root, *setting;
  int number;
  char str[1024] = {0};
  float value = 0;

  root = config_root_setting(cfg);

  GetTemperatureF(&value);
  setting = config_setting_get_member(root, "Temperature");
  config_setting_set_float(setting, value);
  PrintInLog(log, "Temperature");

  /* Write out the new configuration. */
  if(! config_write_file(cfg, output_file)){
    PrintInLog(log, "Can't update configuration!");
  } else {
    PrintInLog(log, "Configuration was updated.");
  }
}

void Shutter(int mode, FILE** log){
  if (mode != 0) mode = 2;
  int InternalShutter = 0;  // This flag shows if camera has an internal shutter
  IsInternalMechanicalShutter(&InternalShutter); // Checking existance of internal shutter
  if (InternalShutter){
    // TTL to open is high (1), mode is fully-auto, time to open and close is about 50ms
    SetShutter(1, mode, 50, 50);
  } else {
    // TTL to open is high (1), mode is fully-auto,
    // time to open and close is about 50ms, external shutter mode is fully-auto
    SetShutterEx(1, mode, 50, 50, 0);
  }
  if (mode != 0) PrintInLog(log, "Shutter is in Auto mode.");
  else PrintInLog(log, "Shutter is closed.");
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
  PrintInLog(log, "PID");

  GetCameraSerialNumber(&number);
  setting = config_setting_add(root, "Serial", CONFIG_TYPE_INT);
  config_setting_set_int(setting, number);
  PrintInLog(log, "Serial");

  GetHeadModel(str);
  setting = config_setting_add(root, "Model", CONFIG_TYPE_STRING);
  config_setting_set_string(setting, str);
  PrintInLog(log, "Model");
  static const char *output_file = strcat(str, ".info");

  GetTemperatureF(&value);
  setting = config_setting_add(root, "Temperature", CONFIG_TYPE_FLOAT);
  config_setting_set_float(setting, value);
  PrintInLog(log, "Temperature");



  /* Write out the new configuration. */
  if(! config_write_file(cfg, output_file)){
    PrintInLog(log, "Can't update configuration!");
  } else {
    PrintInLog(log, "Configuration was updated.");
  }
}
