#include <stdio.h>
#include <stdlib.h>

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

#include <unistd.h>

#include "fitsio.h"

#include "atmcdLXd.h"

int init();
int loop();

int main(int argc, char* argv[]){
	int status = 0;

	status = init();

	// do {
	// 	status = loop();
	// } while (status == -1);

	return 0;
}

int init(){
    unsigned long error;
    int xpix, ypix;
    char camera_name[1024];
    int desired_temperature = 0;
    int current_temperature = 0;

    //Initialize CCD
    error = Initialize("/usr/local/etc/andor");
	if(error!=DRV_SUCCESS){
		cout << "Initialisation error...exiting" << endl;
		return(1);
	}
	sleep(2);

	error = GetDetector(&xpix, &ypix);

	// error = GetHeadModel(&camera_name);

	do {
		cout << "T: ";
		cin >> desired_temperature;
		error = SetTemperature(desired_temperature);
	} while (error == DRV_P1INVALID);

	error = CoolerON();

	do {
		error = GetTemperature(&current_temperature);
		cout << "\rCurrent temperature is " << current_temperature << "       ";
		sleep(1);
	} while (error != DRV_TEMP_STABILIZED);

	cout << endl << "Desired temperature is achieved!" << endl;

	desired_temperature = 15;
	error = SetTemperature(desired_temperature);
	error = CoolerOFF();

	do {
		error = GetTemperature(&current_temperature);
		cout << "\rCurrent temperature is " << current_temperature << "      ";
	} while (current_temperature < desired_temperature);

	error = ShutDown();

	return 0;
}

