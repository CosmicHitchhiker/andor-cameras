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
	if (value.compare("nan") == 0) {
		types.push_back('n');
		values.push_back(value);
	} else {
		types.push_back(type);
		values.push_back(value);
	}
	comments.push_back(comment);
}

std::string HeaderValues::parseString(std::string str){
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

	parsed.push_back(buffer.at(0));	// Тип
	parsed.push_back(buffer.at(1)); // Ключ

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
					parsed.back().pop_back(); // Удаляем закрывающие кавычки
					parsed.back().pop_back(); // Удаляем пробел
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
		currWord++; // БЕГАЕМ ПОКА НЕ КОНЧИТСЯ СТРОКА!!!
	}

	checkType(parsed.at(0)[0], parsed.at(2));
	this->addKey(parsed.at(1), parsed.at(0)[0], parsed.at(2), parsed.at(3));
	string retval = string("KEY=")+parsed.at(1)+" TYPE="+parsed.at(0)[0]+" VALUE=";
	if (parsed.at(0)[0]=='s') retval += '\'';
	retval += parsed.at(2);
	if (parsed.at(0)[0]=='s') retval += '\'';
	retval += " COMMENT=\'"+parsed.at(3)+'\'';
	return retval;
}

void HeaderValues::printAll(){
	for (int i = 0; i < n; ++i)
	{
		cout << "Key: " << keys.at(i) << " ";
		cout << "Value: " << values.at(i).c_str() << " ";
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
			fits_update_key(image, TINT, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 'f'){
			float val = stof(values.at(i));
			fits_update_key(image, TFLOAT, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 'd'){
			double val = stod(values.at(i));
			fits_update_key(image, TDOUBLE, keys.at(i).c_str(), &val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 's'){
			char* val = (char *)values.at(i).c_str();
			fits_update_key(image, TSTRING, keys.at(i).c_str(), val, comments.at(i).c_str(), &status);
		} else if (types.at(i) == 'x'){
			fits_delete_key(image, keys.at(i).c_str(), &status);
		} else if (types.at(i) == 'n'){
			fits_update_key_null(image, keys.at(i).c_str(), comments.at(i).c_str(), &status);
		}
	}
	fits_close_file(image, &status);
}

void HeaderValues::checkType(char type, string value){
	/* Эта функция вызовет исключение, если тип некорректен */
	// try {
		if (type == 'i'){
			int val = stoi(value);
		} else if (type == 'f'){
			float val = stof(value);
		} else if (type == 'd'){
			double val = stod(value);
		} else if (type == 's'){
			char* val = (char *)value.c_str();
		}
	// }
}
