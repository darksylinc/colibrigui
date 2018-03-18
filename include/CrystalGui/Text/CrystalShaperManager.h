
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"

#include <vector>

typedef struct FT_LibraryRec_  *FT_Library;
typedef int  FT_Error;

namespace Crystal
{
	class ShaperManager
	{
		FT_Library	m_ftLibrary;
		CrystalManager	*m_crystalManager;

	public:
		FT_Library getFreeTypeLibrary() const		{ return m_ftLibrary; }
		LogListener* getLogListener() const;

		static const char* getErrorMessage( FT_Error errorCode );
	};
}
