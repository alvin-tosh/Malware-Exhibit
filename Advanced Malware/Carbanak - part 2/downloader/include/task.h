#pragma once

#include "core\core.h"
#include "core\pipe.h"

//выполнение команд

namespace Task
{

bool Init();

//выполнение команды cmd.
//len нужно обязательно указать
//функция запускает выполнение команды в отдельном потоке
bool ExecCmd( const char* cmd, int len );

}
