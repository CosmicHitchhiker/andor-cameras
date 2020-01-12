#include "header_values.h"
#include <iostream>

using namespace std;

HeaderValues::HeaderValues(){
	n = 0;
	keys.clear();
	types.clear();
	values.clear();
	comments.clear();
}

HeaderValues::~HeaderValues(){
}

void HeaderValues::addKey(std::string key, char type, std::string value, std::string comment){
	n+=1;
	keys.push_back(key);
	types.push_back(type);
	values.push_back(value);
	comments.push_back(comment);
}

void HeaderValues::parseString(std::string str){
	vector<string> buffer;
	vector<string> parsed;
	boost::split(buffer, str, boost::is_any_of(" \t\n\0"));

	/* IMPORTANT!!!
	  Предполагается, что str имеет следующий формат:
	  i|d|s|f|x KEY [VALUE [COMMENT]] */

	// Если не указан тип - считаем строкой
	// БУДЕТ НЕКОРРЕКТНО РАБОТАТЬ, если есть тольк KEY, состоящий из idscfx
	// Если передан неправильный тип, тип станет ключом!!!
	if (buffer.at(0).length() != 1 or buffer.at(0).find_first_not_of("idsfx") != std::string::npos){
		buffer.insert(buffer.begin(), string("s"));
	}

	parsed.push_back(buffer.at(0));
	parsed.push_back(buffer.at(1));

	// Если есть что-то кроме типа и ключа - ищем кавычки
	int currWord = 2;
	parsed.push_back(string(""));
	if (buffer.size() > 2){
		if (buffer.at(currWord)[0] == '\"'){
			buffer.at(currWord).erase(0, 1); // Удаляем открывающие кавычки
			while(currWord < buffer.size()){
				parsed.back() += buffer.at(currWord);
				parsed.back() += " ";
				if (buffer.at(currWord).back() == '\"') {
					parsed.back().pop_back();
					parsed.back().pop_back();
					break;
				}
				currWord++;
			}
		} else {
			parsed.back() += buffer.at(currWord);
		}
	}
	currWord++;

	parsed.push_back(string(""));
	while(currWord < buffer.size()){
		parsed.back() += buffer.at(currWord);
		parsed.back() += " ";
		currWord++;
	}


	this->addKey(parsed[1], parsed.at(0)[0], parsed[2], parsed[3]);
}

void HeaderValues::printAll(){
	for (int i = 0; i < n; ++i)
	{
		cout << "Key: " << keys.at(i) << " ";
		cout << "Value: " << values.at(i) << " ";
		cout << "Comment: " << comments.at(i) << " ";
		cout << "\n";
	}
}

void HeaderValues::update(std::string filename) {
	int status = 0;
	fitsfile* image;
	fits_open_data(&image, filename.c_str(), READWRITE, &status);
	for (int i=0; i < n; i++){
		if (types.at(i) == 'i'){
			int val = stoi(values.at(i));
			fits_write_key(image, TINT, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 'f'){
			float val = stof(values.at(i));
			fits_write_key(image, TFLOAT, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 'd'){
			double val = stod(values.at(i));
			fits_write_key(image, TDOUBLE, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 's'){
			const char* val = values.at(i).c_str();
			fits_write_key(image, TSTRING, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 'x'){
			fits_delete_key(image, keys.at(i).c_str(), &status);
		}
	}
	fits_close_file(image, &status);
}

// i KEY "VA LU E" COMMENT