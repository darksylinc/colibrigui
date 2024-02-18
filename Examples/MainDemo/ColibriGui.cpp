
#include "ExamplesCommon.h"

#include "ColibriGuiGameState.h"

#include "System/MainEntryPoints.h"

namespace Demo
{
	void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
										 GraphicsSystem **outGraphicsSystem,
										 GameState **outLogicGameState, LogicSystem **outLogicSystem )
	{
		ColibriGuiGameState *gfxGameState = new ColibriGuiGameState( "Colibri GUI Main Demo" );

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

	const char *MainEntryPoints::getWindowTitle() { return "Colibri GUI Main Demo"; }
}  // namespace Demo
