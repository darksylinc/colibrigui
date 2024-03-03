
#include "ExamplesCommon.h"

#include "OffScreenCanvas2DGameState.h"

#include "System/MainEntryPoints.h"

namespace Demo
{
	void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
										 GraphicsSystem **outGraphicsSystem,
										 GameState **outLogicGameState, LogicSystem **outLogicSystem )
	{
		OffScreenCanvas2DGameState *gfxGameState =
			new OffScreenCanvas2DGameState( "OffScreen Canvas 2D Demo" );

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

	const char *MainEntryPoints::getWindowTitle() { return "OffScreen Canvas 2D Demo"; }
}  // namespace Demo
