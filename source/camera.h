#include <fstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <numeric>
#include <algorithm>

#include "atmcdLXd.h"
#include <libconfig.h++>

#include "log.h"
#include "header_values.h"

#ifndef CAMERA_H
#define CAMERA_H

using namespace libconfig;


class Camera
{
  public:
    Camera(bool isParent = true);
    ~Camera(){}
    virtual void init(Log* logFile, Config* ini);
    std::string getModel();
    std::string parseCommand(std::string message);
    /** Return 
      0 if exposure in progress
      1 if exposure finished
    */
    virtual bool imageReady();
    virtual std::string saveImage();
    virtual void updateStatement();
    virtual void endWork();
    /// Возвращает #expstarted
    bool expStarted() { return expstarted;}

  protected:
    virtual void getShiftSpeedsInfo();
    virtual std::string startExposure();
    virtual void setTemperature();
    virtual void setShutterMode();
    void andorInit();
    std::string fileName();
    void readIni(Config *ini);
    std::string bin(int hbin, int vbin);
    std::string setCrop(int xmin, int xmax, int ymin, int ymax);
    std::string textStatus(int);
    virtual void speed(std::string sp);
    virtual void vspeed(std::string sp);

  protected:
  	/// Статус камеры (см. Andor SDK)
    int status;
    /// Началась ли экспозиция
    bool expstarted;
    /// Время начала экспозиции
    time_t startTime;
    /// Температура камеры
    float temperature;
    /// Температура, до которой должна охладиться/нагреться камера
    int targetTemperature;
    /// Минимальная достижимая температура охладителя
    int minT;
    /// Максимальная достижимая температура охладителя
    int maxT;
    /// Статус охлаждения
    unsigned int temperatureStatus;

    /// Время экспозиции
    float exposureTime;

    /// Полная ширина матрицы
    int width;
    /// Полная высота матрицы
    int height;
    /// Заданное горизонтальное бинирование
    int hBin;
    /// Заданное вертикальное бинирование
    int vBin;
    /// Какую область камеры использовать для съёмки (Xmin, Xmax, Ymix, Ymax)
    std::vector<int> crop;

    /// Номер выбранной в данный момент скорости горизонтального сдвига
    int hssNo;
    /// Номер минимальной скорости горизонтального сдвига
    int min_hss_No;
    /// Номер максимальной скорости горизонтального сдвига
    int max_hss_No;
    /// Номер выбранной в данный момент скорости вертикального сдвига
    int vssNo;
    /// Номер минимальной скорости вертикального сдвига
    int min_vss_No;
    /// Номер максимальной скорости вертикального сдвига
    int max_vss_No;

    /// Номер текущего режима считывания
    // Соответствие номера режиму см. в переменной readModes
    int readMode;
    /// Номер текущего режима накопления
    // Соответствие номера режиму см. в переменной acquisitionModes
    int acquisitionMode;
    /// Номер текущего режима работы затвора
    // См. переменную shutterModes
    int shutterMode;
    /// Модель камеры
    std::string model;
    /// Имя, под которым будет сохранён текущий кадр
    std::string fname;
    /// Имеется ли у камеры встроенный затвор
    int isInternalShutter;
    /// Время закрытия затвора (мс)
    int shutterCloseTime;
    /// Время открытия затвора (мс)
    int shutterOpenTime;

    /// В какую директорию сохранять текущие наблюдения
    std::string writeDirectory;
    /// Префикс в имени сохраняемого файла
    std::string prefix;
    /// Постфикс в имени сохраняемого файла
    std::string postfix;
    /// Тип данных сохраняемого fits
    // См. переменную dataTypes
    int dataType;

    /// Массив значений хедера
    HeaderValues header;
    /// Указатель на лог-файл
    Log* log;
    /// Файл с текущими параметрами камеры камеры
    Config cfg;
    /// Имя файла с текущими параметрами
    std::string configName;

    /// Возможные режимы считывания
    std::vector<std::string> readModes;
    /// Возможные режимы накопления
    std::vector<std::string> acquisitionModes;
    /// Возможные режимы работы затвора
    std::vector<std::string> shutterModes;
    /// То же, что #shutterModes, но в фомате статус-сообщений
    std::vector<std::string> shutterMStatus;
    /// Возможные скорости горизонтального сдвига
    std::vector<float> hss;
    /// Возмодные скорости вертикального сдвига
    std::vector<float> vss;

};

#endif // CAMERA_H
