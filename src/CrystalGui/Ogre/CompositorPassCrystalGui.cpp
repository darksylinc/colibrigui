
#include "CrystalGui/Ogre/CompositorPassCrystalGui.h"
#include "CrystalGui/Ogre/CompositorPassCrystalGuiDef.h"
#include "CrystalGui/CrystalManager.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

namespace Ogre
{
	CompositorPassCrystalGui::CompositorPassCrystalGui( const CompositorPassCrystalGuiDef *definition,
														SceneManager *sceneManager,
														const RenderTargetViewDef *rtv,
														CompositorNode *parentNode,
														Crystal::CrystalManager *crystalManager ) :
		CompositorPass( definition, parentNode ),
		mSceneManager( sceneManager ),
		m_crystalManager( crystalManager ),
		mDefinition( definition )
	{
		initialize( rtv );
	}
	//-----------------------------------------------------------------------------------
	void CompositorPassCrystalGui::execute( const Camera *lodCamera )
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

		m_crystalManager->prepareRenderCommands();
		m_crystalManager->render();

		if( listener )
			listener->passPosExecute( this );

		profilingEnd();
	}
}
