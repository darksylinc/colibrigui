
#include "ColibriGui/Ogre/CompositorPassColibriGuiProvider.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiDef.h"
#include "ColibriGui/Ogre/CompositorPassColibriGui.h"

namespace Ogre
{
	CompositorPassColibriGuiProvider::CompositorPassColibriGuiProvider( Colibri::ColibriManager *colibriManager ) :
		m_colibriManager( colibriManager )
	{
	}
	//-------------------------------------------------------------------------
	CompositorPassDef* CompositorPassColibriGuiProvider::addPassDef(
			CompositorPassType passType, IdString customId,
			CompositorTargetDef *parentTargetDef, CompositorNodeDef *parentNodeDef )
	{
		if( customId == "colibri_gui" )
			return OGRE_NEW CompositorPassColibriGuiDef( parentTargetDef );

		return 0;
	}
	//-------------------------------------------------------------------------
	CompositorPass* CompositorPassColibriGuiProvider::addPass( const CompositorPassDef *definition,
															   Camera *defaultCamera,
															   CompositorNode *parentNode,
															   const RenderTargetViewDef *rtvDef,
															   SceneManager *sceneManager )
	{
		COLIBRI_ASSERT( dynamic_cast<const CompositorPassColibriGuiDef*>( definition ) );
		const CompositorPassColibriGuiDef *colibriGuiDef =
				static_cast<const CompositorPassColibriGuiDef*>( definition );
		return OGRE_NEW CompositorPassColibriGui( colibriGuiDef, sceneManager, rtvDef,
												  parentNode, m_colibriManager );
	}
}
