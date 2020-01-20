#include "header_values.h"
#include <string>
#include <iostream>

using namespace std;

int main(){
	string str;
	getline(cin, str);
	HeaderValues hv;
	hv.parseString(str);
	hv.update("sample.fits");
	hv.printAll();
	return 0;
}