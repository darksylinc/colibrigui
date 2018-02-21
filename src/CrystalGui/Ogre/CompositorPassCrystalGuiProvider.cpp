
#include "CrystalGui/Ogre/CompositorPassCrystalGuiProvider.h"
#include "CrystalGui/Ogre/CompositorPassCrystalGuiDef.h"
#include "CrystalGui/Ogre/CompositorPassCrystalGui.h"

namespace Ogre
{
	CompositorPassCrystalGuiProvider::CompositorPassCrystalGuiProvider( Crystal::CrystalManager *crystalManager ) :
		m_crystalManager( crystalManager )
	{
	}
	//-------------------------------------------------------------------------
	CompositorPassDef* CompositorPassCrystalGuiProvider::addPassDef(
			CompositorPassType passType, IdString customId,
			CompositorTargetDef *parentTargetDef, CompositorNodeDef *parentNodeDef )
	{
		if( customId == "crystal_gui" )
			return OGRE_NEW CompositorPassCrystalGuiDef( parentTargetDef );

		return 0;
	}
	//-------------------------------------------------------------------------
	CompositorPass* CompositorPassCrystalGuiProvider::addPass( const CompositorPassDef *definition,
															   Camera *defaultCamera,
															   CompositorNode *parentNode,
															   const RenderTargetViewDef *rtvDef,
															   SceneManager *sceneManager )
	{
		CRYSTAL_ASSERT( dynamic_cast<const CompositorPassCrystalGuiDef*>( definition ) );
		const CompositorPassCrystalGuiDef *crystalGuiDef =
				static_cast<const CompositorPassCrystalGuiDef*>( definition );
		return OGRE_NEW CompositorPassCrystalGui( crystalGuiDef, sceneManager, rtvDef,
												  parentNode, m_crystalManager );
	}
}
