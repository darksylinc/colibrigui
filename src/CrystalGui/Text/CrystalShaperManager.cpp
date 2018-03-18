
#include "CrystalGui/Text/CrystalShaperManager.h"
#include "CrystalGui/CrystalManager.h"

#include "ft2build.h"
#include "freetype/freetype.h"

namespace Crystal
{
	LogListener* ShaperManager::getLogListener() const
	{
		return m_crystalManager->getLogListener();
	}
	//-------------------------------------------------------------------------
	const char* ShaperManager::getErrorMessage( FT_Error errorCode )
	{
		#undef __FTERRORS_H__
		#define FT_ERRORDEF( e, v, s )  case e: return s;
		#define FT_ERROR_START_LIST     switch( errorCode ) {
		#define FT_ERROR_END_LIST       }
		#include FT_ERRORS_H
		return "(Unknown error)";
	}
}
