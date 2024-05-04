
#include "ExamplesCommon.h"

#include "OffScreenCanvas3DGameState.h"

#include "System/MainEntryPoints.h"

namespace Demo
{
	void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
										 GraphicsSystem **outGraphicsSystem,
										 GameState **outLogicGameState, LogicSystem **outLogicSystem )
	{
#if OGRE_VERSION < OGRE_MAKE_VERSION( 2, 3, 0 )
		OGRE_EXCEPT( Ogre::Exception::ERR_NOT_IMPLEMENTED, "This sample requires OgreNext 2.3 or newer",
					 "MainEntryPoints::createSystems" );
#endif
		OffScreenCanvas3DGameState *gfxGameState =
			new OffScreenCanvas3DGameState( "OffScreen Canvas 3D Demo" );

		GraphicsSystem *graphicsSystem = new ColibriGuiGraphicsSystem( gfxGameState );

		gfxGameState->_notifyGraphicsSystem( graphicsSystem );

		*outGraphicsGameState = gfxGameState;
		*outGraphicsSystem = graphicsSystem;
	}

	void MainEntryPoints::destroySystems( GameState *graphicsGameState, GraphicsSystem *graphicsSystem,
										  GameState *logicGameState, LogicSystem *logicSystem )
	{
		delete graphicsSystem;
		delete graphicsGameState;
	}

	const char *MainEntryPoints::getWindowTitle() { return "OffScreen Canvas 3D Demo"; }
}  // namespace Demo
