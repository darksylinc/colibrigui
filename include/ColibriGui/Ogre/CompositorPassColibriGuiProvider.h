
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "OgrePrerequisites.h"

#include "Compositor/Pass/OgreCompositorPassProvider.h"

namespace Ogre
{
	/** @ingroup Api_Backend
	@class CompositorPassColibriGuiProvider
		Generates the CompositorPassColibriGui pass needed to render Colibri UI
		Also ensures the compositor scripts recognize the pass type "colibri_gui"
		i.e.
		@code
			pass custom colibri_gui
			{
			}
		@endcode
	*/
	class CompositorPassColibriGuiProvider : public CompositorPassProvider
	{
		Colibri::ColibriManager *m_colibriManager;

	public:
		CompositorPassColibriGuiProvider( Colibri::ColibriManager *colibriManager );

		/** Called from CompositorTargetDef::addPass when adding a Compositor Pass of type 'custom'
		@param passType
		@param customId
			Arbitrary ID in case there is more than one type of custom pass you want to implement.
			Defaults to IdString()
		@param rtIndex
		@param parentNodeDef
		@return
		*/
		CompositorPassDef *addPassDef( CompositorPassType passType, IdString customId,
									   CompositorTargetDef *parentTargetDef,
									   CompositorNodeDef   *parentNodeDef ) override;

		/** Creates a CompositorPass from a CompositorPassDef for Compositor Pass of type 'custom'
		@remarks    If you have multiple custom pass types then you will need to use dynamic_cast<>()
					on the CompositorPassDef to determine what custom pass it is.
		*/
		CompositorPass *addPass( const CompositorPassDef *definition, Camera *defaultCamera,
								 CompositorNode *parentNode, const RenderTargetViewDef *rtvDef,
								 SceneManager *sceneManager ) override;

#if OGRE_VERSION >= OGRE_MAKE_VERSION( 3, 0, 0 )
		void translateCustomPass( ScriptCompiler *compiler, const AbstractNodePtr &node,
								  IdString customId, CompositorPassDef *customPassDef ) override;
#endif
	};
}  // namespace Ogre
