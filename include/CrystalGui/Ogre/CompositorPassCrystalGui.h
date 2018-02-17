
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"

#include "OgrePrerequisites.h"
#include "Compositor/Pass/OgreCompositorPass.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
	class CompositorPassCrystalGuiDef;

	/** Implementation of CompositorPass
	@author
		Matias N. Goldberg
	@version
		1.0
	*/
	class _OgreExport CompositorPassCrystalGui : public CompositorPass
	{
	protected:
		SceneManager    *mSceneManager;
		Crystal::CrystalManager *m_crystalManager;

	public:
		CompositorPassCrystalGui( const CompositorPassCrystalGuiDef *definition,
								  SceneManager *sceneManager,
								  const RenderTargetViewDef *rtv, CompositorNode *parentNode,
								  Crystal::CrystalManager *crystalManager );

		virtual void execute( const Camera *lodCamera );

	private:
		CompositorPassCrystalGuiDef const *mDefinition;
	};
}

#include "OgreHeaderSuffix.h"
