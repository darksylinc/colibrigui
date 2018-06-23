
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "OgrePrerequisites.h"
#include "Compositor/Pass/OgreCompositorPass.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
	class CompositorPassColibriGuiDef;

	/** Implementation of CompositorPass
	@author
		Matias N. Goldberg
	@version
		1.0
	*/
	class CompositorPassColibriGui : public CompositorPass
	{
	protected:
		SceneManager    *mSceneManager;
		Colibri::ColibriManager *m_colibriManager;

	public:
		CompositorPassColibriGui( const CompositorPassColibriGuiDef *definition,
								  SceneManager *sceneManager,
								  const RenderTargetViewDef *rtv, CompositorNode *parentNode,
								  Colibri::ColibriManager *colibriManager );

		virtual void execute( const Camera *lodCamera );

	private:
		CompositorPassColibriGuiDef const *mDefinition;
	};
}

#include "OgreHeaderSuffix.h"
