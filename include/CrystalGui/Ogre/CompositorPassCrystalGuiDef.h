
#pragma once

#include "OgreHeaderPrefix.h"

#include "Compositor/Pass/OgreCompositorPassDef.h"

namespace Ogre
{
	class CompositorPassCrystalGuiDef : public CompositorPassDef
	{
	public:
		CompositorPassCrystalGuiDef( CompositorTargetDef *parentTargetDef ) :
			CompositorPassDef( PASS_CUSTOM, parentTargetDef )
		{
		}
	};
}

#include "OgreHeaderSuffix.h"
