// TODO: make an extension for empty commands

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef __GNUC__
#  if(__GNUC__ > 3 || __GNUC__ ==3)
#	define _GNUC3_
#  endif
#endif

#ifdef _GNUC3_
#  include <iostream>
#  include <fstream>
   using namespace std;
#else
#  include <iostream.h>
#  include <fstream.h>
#endif

#include "fitsio.h"
#include "atmcdLXd.h"

int CameraSelect (int iNumArgs, char* szArgList[]);
int temp(int T);
int info();
int image(float exp_time);
int series();
int shutter(int mode);
int end();

int main(int argc, char* argv[]){
  // Select mentioned camera
  // If no camera is mentioned, first camera is selected
  // CHECK PREVIOUS FACT!!!
  // if (CameraSelect (argc, argv) < 0) {
  //     cout << "*** CAMERA SELECTION ERROR ***" << endl;
  //     return -1;
  // }

  // create Socket
  int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener < 0){
		puts("Can't create socket\n");
		return 1;
	}

  // camera variables
  unsigned long status;   // status of the last function
  // int xpix, ypix;        // size of the camera
  // int temperature;
  // float exp_time = 0.1;  // exposure time in seconds
  char *command;

  // socket variables
  int sock;
	struct sockaddr_in addr;
	char bufer[1024] = {0};
	int port_number = atoi(argv[1]);

  addr.sin_family = AF_INET;
	addr.sin_port = htons(port_number);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		puts("Can't bind socket\n");
		return 2;
	}
	puts("Socket is bound\n");
	listen(listener, 1);
  puts("Listening...\n");

  // status = Initialize("/usr/local/etc/andor");
	// if(status!=DRV_SUCCESS){
	// 	cout << "Initialisation error...exiting" << endl;
	// 	return(1);
	// }
  // sleep(2);   // wait for the end of the initialization
  //
  // SetReadMode(4);   // set mode to "Image"
  //
  // GetDetector(&width, &height);   // get Detector shapes
  SetShutter(1,0,50,50);

  do {
    sock = accept(listener, 0, 0);
    if(sock < 0)
  	{
  		puts("Can't connect to the client\n");
  		return 3;
  	}
    read(sock, bufer, 1024);
    puts(bufer);
    command = strtok(bufer, " \n\0");
    if (command == NULL) printf("error\n");
    else if (strcmp(command,"temp") == 0) status = temp(atoi(strtok(NULL, " \n\0")));
    else if(strcmp(command,"status") == 0) status = info();
    else if (strcmp(command,"image") == 0) status = image(atof(strtok(NULL, " \n\0")));
    else if (strcmp(command,"series") == 0) status = series();
    else if (strcmp(command,"shutter") == 0) status = shutter(atoi(strtok(NULL, " \n\0")));
    else if (strcmp(command,"abort") == 0) {
      status = end();
      if (status == 1) break;
    } else printf("Command %s doesn't exist\n", command);



  } while(1);







	return 0;
}


int CameraSelect (int iNumArgs, char* szArgList[]){
  if (iNumArgs == 2) {

    at_32 lNumCameras;
    GetAvailableCameras(&lNumCameras);
    int iSelectedCamera = atoi(szArgList[1]);

    if (iSelectedCamera < lNumCameras && iSelectedCamera >= 0) {
      at_32 lCameraHandle;
      GetCameraHandle(iSelectedCamera, &lCameraHandle);
      SetCurrentCamera(lCameraHandle);
      return iSelectedCamera;
    }
    else
      return -1;
  }
  return 0;
}

int temp(int T){
  printf("%d\n", T);
  fputs("hohoho\n", stdout);
  return 0;
}

int info(){
  puts("hohoho");
  return 0;
}

int image(float exp_time){
  fitsfile *file;
  int status = 0;
  long firstpix[2] = {1, 1};
  int height, width;
  GetDetector(&width, &height);
  long naxis[2] = {width, height};
  int *img;
  SetAcquisitionMode(3);
  SetExposureTime(exp_time);
  SetImage(1,1,1,width,1,height);

  StartAcquisition();

  img = (int *) malloc(height * width * sizeof(int));
  fits_create_file(&file, "\!test.fits", &status);
  fits_create_img(file, LONG_IMG, 2, naxis, &status);

  //Loop until acquisition finished
  GetStatus(&status);
  while(status==DRV_ACQUIRING) GetStatus(&status);

  GetAcquiredData(img, width*height);

  fits_write_pix(file, TINT, firstpix, width*height, img, &status);

  fits_close_file(file, &status);

  free(img);

  puts("image");
  return 0;
}

int series(){
  puts("hohoho");
  return 0;
}

int shutter(int mode){
  i
}

int end(){
  puts("hohoho");
  return 1;
}
