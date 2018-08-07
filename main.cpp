// TODO: make an extension for empty commands

#include "main.h"

#define DEFAULT_PORT 1234

int CameraSelect (int iNumArgs, char* szArgList[]);
int temp(int T);
int info();
int image(float t, fitsfile* file, FILE** log);
int series();
int end();
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
  FILE *log = NULL;
  int fits_status = 0;

  int listener = 0;
  struct sockaddr_in addr;
  int port_number = DEFAULT_PORT;
  if (argc > 1)
    port_number = atoi(argv[1]);
  char client_message[1024] = {0};
  char *command;

  // int cur_temp, tar_temp, max_temp, min_temp;
  // int xpix, ypix;

  time_t cur_time;
  struct tm curr_time;

  fitsfile *template_fits;

  LogFileInit(&log);

  SocketInit(&listener, &addr, port_number, &log);

  CameraInit(&log);

  FitsInit(&template_fits);
  
  do{
    GetMessage(listener, client_message);
    client_message = strtok(client_message, "\n");
    PrintInLog(&log, client_message);
    command = strtok(client_message, " \n\0");
    if (command == NULL) PrintInLog(&log, "Empty command");
    else if (strcmp(command,"image") == 0) fits_status = image(atof(strtok(NULL, " \n\0")), template_fits, &log);
    else if (strcmp(command,"save") == 0) fits_status = save_fits(template_fits);
  } while(strcmp(command,"exit") != 0);

  cur_time = time(NULL);
  fprintf(log, "\nLog ending %s\n", ctime(&cur_time));
  close(listener);
  fclose(log);
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

  time(&cur_time);
  curr_time = localtime(&cur_time);

  strftime(buffer,20,"%T %h %d ",curr_time);
  fprintf(*log, "%s: ", buffer);
  fprintf(*log, "%s\n", message);
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
  fits_create_file(file, "\!image.fits", &status);
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
  long naxis[2] = {width, height};
  SetImage(1,1,1,width,1,height);
  StartAcquisition();
  int imageData[height*width];

  int status;
  int fits_status = 0;

  //Loop until acquisition finished
  GetStatus(&status);
  while(status==DRV_ACQUIRING) GetStatus(&status);
  PrintInLog(log, "Image is acquired");

  GetAcquiredData(imageData, width*height);
  PrintInLog(log, "Image is written in the array");

  imageData[1] = 0;

  long firstpix[2] = {1, 1};
  fits_movabs_hdu(file, 1, IMAGE_HDU, &fits_status);
  fits_write_pix(file, TINT, firstpix, width*height, imageData, &fits_status);

  //SaveAsFITS("./image3.fits", 2);
  //fits_close_file(file, &fits_status);
  
  return 0;
}

// int series(){
//   puts("hohoho");
//   return 0;
// }

// int end(){
//   puts("hohoho");
//   return 1;
// }
