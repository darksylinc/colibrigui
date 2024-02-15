
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgrePixelFormatGpu.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	class CompositorPassColibriGuiProvider;
}

namespace Colibri
{
	/** @ingroup Controls
	@class OffScreenCanvas
		This class allows you to have an independent ColibriManager that can render to an arbitrary
		texture that you can later display to either 3D or even in a Colibri Renderable as a widget.

		You can retrieve this secondary ColibriManager via getSecondaryManager().
		Note that you must not mix widgets from different ColibriManager
		(i.e. use mgr1->createWidget( parentCreatedWithMgr2 )).

		Use cases include:
			1. VR with multiple 3D panels.
			2. Independent 2D UI (e.g. minimaps).
			3. Fake in-game UI.
			4. Generating text that can later be displayed in 3D.
	*/
	class OffScreenCanvas
	{
	protected:
		ColibriManager *m_secondaryManager;

		Ogre::CompositorWorkspace *colibri_nullable m_workspace;

		Ogre::TextureGpu *colibri_nullable m_canvasTexture;

		void createWorkspaceDefinition();
		void destroyWorkspace();

	public:
		OffScreenCanvas( ColibriManager *primaryManager );
		~OffScreenCanvas();

		/// Returns a ColibriManager that is NOT the same as the primary one.
		/// Use that ColibriManager pointer to manage the windows and widgets from
		/// this off-screen canvas.
		///
		/// You can also use it to handle its keyboard/mouse state.
		///
		/// The log manager and colibri listeners are shared though.
		ColibriManager *getSecondaryManager() { return m_secondaryManager; }

		void createTexture( uint32_t width, uint32_t height,
							Ogre::PixelFormatGpu pixelFormat = Ogre::PFG_RGBA8_UNORM_SRGB );

		/** Creates a workspace (based on a programatically-created workspace definition
			that only has one pass which is for Colibri and clears the colour to black).
		@param colibriCompositorProvider
			Provider so that we can temporarily swap out to the secondary ColibriManager
			by calling Ogre::CompositorPassColibriGuiProvider_setColibriManager.
		@param camera
			The camera to pass to the workspace.
		*/
		void createWorkspace( Ogre::CompositorPassColibriGuiProvider *colibriCompositorProvider,
							  Ogre::Camera                           *camera );

		/** Updates and renders the off-screen canvas.
		@param timeSinceLast
			Time in seconds since last call. Used for animations (if any) as well as anything
			else that requires time (like time stroke repeats).
		*/
		void updateCanvas( const float timeSinceLast );

		Ogre::TextureGpu *colibri_nullable getCanvasTexture() { return m_canvasTexture; }
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
