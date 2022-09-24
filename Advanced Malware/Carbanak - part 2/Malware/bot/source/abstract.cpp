#include "core\core.h"
#include "core\abstract.h"
#include "main.h"

namespace Abstract
{

bool GetUid( StringBuilder& uid )
{
	uid = Config::UID;
	return true;
}

}
