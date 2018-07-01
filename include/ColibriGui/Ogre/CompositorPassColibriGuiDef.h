
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
		bool mSetsResolution;

	public:
		CompositorPassColibriGuiDef( CompositorTargetDef *parentTargetDef ) :
			CompositorPassDef( PASS_CUSTOM, parentTargetDef ),
			mSetsResolution( true )
		{
			mProfilingId = "Colibri Gui";
		}
	};
}

#include "OgreHeaderSuffix.h"
