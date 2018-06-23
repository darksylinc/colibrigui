
#include "ColibriGui/Ogre/CompositorPassColibriGui.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiDef.h"
#include "ColibriGui/ColibriManager.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

namespace Ogre
{
	CompositorPassColibriGui::CompositorPassColibriGui( const CompositorPassColibriGuiDef *definition,
														SceneManager *sceneManager,
														const RenderTargetViewDef *rtv,
														CompositorNode *parentNode,
														Colibri::ColibriManager *colibriManager ) :
		CompositorPass( definition, parentNode ),
		mSceneManager( sceneManager ),
		m_colibriManager( colibriManager ),
		mDefinition( definition )
	{
		initialize( rtv );
	}
	//-----------------------------------------------------------------------------------
	void CompositorPassColibriGui::execute( const Camera *lodCamera )
	{
		//Execute a limited number of times?
		if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
		{
			if( !mNumPassesLeft )
				return;
			--mNumPassesLeft;
		}

		profilingBegin();

		CompositorWorkspaceListener *listener = mParentNode->getWorkspace()->getListener();
		if( listener )
			listener->passEarlyPreExecute( this );

		executeResourceTransitions();

		//Fire the listener in case it wants to change anything
		if( listener )
			listener->passPreExecute( this );

		m_colibriManager->prepareRenderCommands();
		m_colibriManager->render();

		if( listener )
			listener->passPosExecute( this );

		profilingEnd();
	}
}
