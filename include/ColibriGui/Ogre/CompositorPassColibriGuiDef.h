
#pragma once

#include "OgreHeaderPrefix.h"

#include "Compositor/Pass/OgreCompositorPassDef.h"

namespace Ogre
{
	/** @ingroup Api_Backend
	@class CompositorPassColibriGuiDef
	*/
	class CompositorPassColibriGuiDef : public CompositorPassDef
	{
	public:
		CompositorPassColibriGuiDef( CompositorTargetDef *parentTargetDef ) :
			CompositorPassDef( PASS_CUSTOM, parentTargetDef )
		{
		}
	};
}

#include "OgreHeaderSuffix.h"
