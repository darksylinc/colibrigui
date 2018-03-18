
#pragma once

#if defined( _MSC_VER ) && _MSC_VER < 1800

#include "math.h"

inline float round( float val )
{
	return (val > 0.0f) ? floor(val + 0.5f) : ceil(val - 0.5f);
}

inline double round( double val )
{
	return (val > 0.0) ? floor(val + 0.5) : ceil(val - 0.5);
}

#endif
