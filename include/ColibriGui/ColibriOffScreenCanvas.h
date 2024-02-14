
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
	*/
	class OffScreenCanvas
	{
	protected:
		ColibriManager *m_secondaryManager;

		Ogre::CompositorWorkspace *colibri_nullable m_workspace;

		Ogre::TextureGpu *colibri_nullable m_canvasTexture;

		void createWorkspaceDefinition();
		void createWorkspace( Ogre::CompositorPassColibriGuiProvider *colibriCompositorProvider );
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
