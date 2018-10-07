// TODO: make an extension for empty commands

#include "main.h"

#define DEFAULT_PORT 1234

int CameraSelect (int iNumArgs, char* szArgList[]);
int temp(int T);
int image(float t, fitsfile* file, FILE** log);
int save_fits(fitsfile* file);
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

  LogFileInit(&log);

  SocketInit(&listener, &addr, port_number, &log);

  CameraInit(&log);

  FitsInit(&template_fits);
  
  do{
    GetMessage(listener, client_message);
    PrintInLog(&log, client_message);
    strcpy(message, client_message);
    command = strtok(client_message, " \n\0");
    if (command == NULL) PrintInLog(&log, "Empty command");
    else if (strcmp(command,"image") == 0) fits_status = image(atof(strtok(NULL, " \n\0")), template_fits, &log);
    else if (strcmp(command,"header") == 0) fits_status = AddHeaderKey(message, template_fits, &log);
  } while(strcmp(command,"exit") != 0);

  time_t cur_time = time(NULL);
  fprintf(log, "\nLog ending %s\n", ctime(&cur_time));
  close(listener);
  fclose(log);
  fits_close_file(template_fits, &fits_status);
  ShutDown();

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
  time_t cur_time = time(NULL);
  struct tm *curr_time = localtime(&cur_time);
  char buffer[20];

  // time(&cur_time);
  // curr_time = localtime(&cur_time);

  strftime(buffer,20,"%T %h %d ",curr_time);
  fprintf(*log, "%s: ", buffer);
  char* msg = strtok(message, "\n");
  fprintf(*log, "%s\n", msg);
  fflush(*log);
}

void SocketInit(int* listener, struct sockaddr_in* addr, int port_number, FILE** log){
  *listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (*listener < 0){
    PrintInLog(log, "Can't create socket");
    exit(1);
  }

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port_number);
  addr->sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(*listener, (struct sockaddr *)addr, sizeof(*addr)) < 0){
    PrintInLog(log, "Can't bind socket");
    exit(2);
  }

  listen(*listener, 1);
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

  //Initialize CCD
  error = Initialize("/usr/local/etc/andor");
  if(error!=DRV_SUCCESS){
    PrintInLog(log, "Initialisation error...exiting");
    exit(1);
  }

  sleep(2); //sleep to allow initialization to complete

  //Set Read Mode to --Image--
  SetReadMode(4);

  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode(3);

  //Set initial exposure time
  SetExposureTime(0.1);

  SetNumberKinetics(1);

  SetKineticCycleTime(2);

  //Initialize Shutter
  SetShutter(1,0,50,50);
}

void FitsInit(fitsfile** file){
  int width, height;
  int status = 0;
  GetDetector(&width, &height);
  long naxis[2] = {width, height};
  fits_create_file(file, "\!header.fits", &status);
  fits_create_img(*file, LONG_IMG, 2, naxis, &status);
}

void mGetDetector(int *x, int *y){
  fitsfile *detector;
  int status = 0;
  fits_open_data(&detector, "../test/detector.fits", READONLY, &status);
  fits_read_key(detector, TINT, "NAXIS1", x, NULL, &status);
  fits_read_key(detector, TINT, "NAXIS2", y, NULL, &status);
}

int save_fits(fitsfile* file){
  fitsfile* result;
  int status = 0;
  fits_create_file(&result, "\!result.fits", &status);
  fits_copy_file(file, result, 1, 1, 1, &status);
  fits_close_file(result, &status);
  return 0;
}

int image(float t, fitsfile* file, FILE** log){
  int width, height;
  GetDetector(&width, &height);
  //long naxis[2] = {width, height};
  SetImage(1,1,1,width,1,height); //horiz bin, vert bin, first xpix, last xpix, first ypix, last ypix
  char* file_name;
  FileName(file_name); 

  StartAcquisition();
  int status;
  //int fits_status = 0;

  //Loop until acquisition finished
  GetStatus(&status);
  while(status==DRV_ACQUIRING) GetStatus(&status);
  PrintInLog(log, "Image is acquired");

  SaveAsFITS(file_name, 2);
  UpdateHeader(file, file_name);
  
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
  char* card;
  fits_open_data(&image, file_name, READWRITE, &status);
  fits_get_hdrspace(file, &n, NULL, &status);
  for (int i=0; i < n; i++){
    fits_read_record(file, i+1, card, &status);
    fits_write_record(image, card, &status);
  }
  fits_close_file(image, &status);
}

void Temperature(int T){
  SetTemperature(T);
}
