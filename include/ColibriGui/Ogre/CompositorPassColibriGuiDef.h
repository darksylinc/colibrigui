
#pragma once

#include "OgreHeaderPrefix.h"

#include "Compositor/Pass/OgreCompositorPassDef.h"

namespace Ogre
{
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
