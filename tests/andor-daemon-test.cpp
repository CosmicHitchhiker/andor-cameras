#include "andor-daemon-test.h"

using namespace std;
using namespace libconfig;

int Daemon(int argc, char* argv[]);
int Main(int argc, char* argv[]);

// Становится true, если попытаться отправить сообщение отвалившемуся клиенту
bool Socket::g_sig_pipe_caught = false;


int main(int argc, char* argv[]) {
  return Daemon(argc, argv);
}

/**
  Запуск программы в режиме демона.

  Порождает дочерний процесс (в котором всё и происходит), не привязанный к
  терминалу.
  Родительский процесс при этом убивается.
  PID дочернего процесса выводится в терминал.
*/
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

/**
  Основной процесс программы.

  Создаёт объекты Camera, Log, Socket и настраивает их в соответствии с ini.
  Создаёт лог-файл, открывает ТСР-сервер, пишет в cfg состояние камеры.
  Обеспечивает передачу сообщений от клиента к обработчику камеры и ответов клиенту.
  Отслеживает разрыв соединения и позволяет клиенту переподключиться.
  После команды EXIT запускает процедуру завершения работы камеры.
*/
int Main(int argc, char* argv[]){
  // Создаёт класс камеры и задаёт некоторые начальные настройки
  VirtualCamera camera;
  string Model = camera.getModel(); // Для виртуальной камеры модель ANDOR_TEST
  // В этот файл пишутся комментарии ко всем действиям программы
  string logName = Model + ".log";
  // Файл с задаваемыми изначально параметрами
  string iniName = Model + ".ini";

  Log log(logName);
  Config ini;

  ini.readFile(iniName.c_str());
  // Номер ТСР порта. Должен быть записан в ini-файле
  int port = ini.lookup("Port");

  // Запуск ТСР-сервера
  Socket sock(port, &log);
  // Начальные настройки камеры: режим съёмки, экспозиция, режим затвора, 
  //   скорость считывания, целевая температура, предустановленные строки
  //   заголовка фитс-файла, параметры имён сохраняемых фалов (префикс,
  //   суффикс, папка для сохранения)
  camera.init(&log, &ini);
  // Изменение имени процесса
  string processName = string(argv[0])+" "+Model+" "+to_string(port);
  size_t argv0_len = strlen(argv[0]);
  size_t procname_len = strlen((char *)processName.c_str());
  size_t max_procname_len = (argv0_len > procname_len) ? (procname_len) : (argv0_len);
  strncpy(argv[0], (char *)processName.c_str(), max_procname_len);
  memset(&argv[0][max_procname_len], '\0', argv0_len - max_procname_len);
  //argv[0] = (char *)processName.c_str();

  // Переменная для хранения сообщения клиента
  string clientMessage = "";
  // Переменная для хранения ответа сервера
  string serverMessage = "";

  // Задержка для уменьшения потребления процессорного времени
  float timeSleep = 0.5;
  // Ждём пока подключится клиент
  while (! sock.acceptConnection()){
    sleep(timeSleep);
  }
  log.print("Client connected");

  // Блок выполняется до сообщения клиента EXIT
  // В нём принимается и обрабатывается сообщение клиента, а параллельно
  // проверяется готовность изображения и не разорвано ли соединение
  while (clientMessage.compare("EXIT")){
    // Затираем предыдущее сообщение
    clientMessage = "";
    // Пока не получим сообщения от клиента
    while (!sock.getMessage(&clientMessage)){
      // Проверяем, не готово ли предыдущее изображение (если была съёмка)
      if (camera.imageReady()) {
        // Если готово - сохраняем и выходим из цикла (отвечаем клиенту)
        serverMessage = camera.saveImage();
        break;
      }
      // Если не идёт экспозиция, а клиент отключился - ждём нового подключения
      // TODO: если изображение стало готово внутри этого цикла - сохранить
      if (!camera.expStarted() && !sock.isClientConnected()){
          log.print("Client is disconnected");
          while (! sock.acceptConnection())
            sleep(timeSleep);
          log.print("Connected");
          Socket::g_sig_pipe_caught = false;
      }
      sleep(timeSleep);
    }
    // Ответ клиенту выдаёт парсер камеры
    // В случае saveImage программа сбда попадает без сообщения клиента
    if (clientMessage!="") serverMessage = camera.parseCommand(clientMessage);
    // Обновляем информацию о состоянии камеры (и записываем в info)
    camera.updateStatement();
    // Отправляем ответ клиенту
    sock.answer(serverMessage.c_str());
  }

  // Процедура завершения работы (аккуратный нагрев)
  camera.endWork();
  // Только после завершения выключаем сервер
  sock.turnOff();
  return (0);
}
