
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

		Example:

		@code
			Colibri::OffScreenCanvas* offscrCanvas = new Colibri::OffScreenCanvas( colibriManager );
			offscrCanvas->createTexture( 512, 512 );
			offscrCanvas->createWorkspace( compositorPassColibriGuiProvider ), camera );

			Colibri::ColibriManager *secondaryMgr = offscrCanvas->getSecondaryManager();
			secondaryMgr->setCanvasSize( Ogre::Vector2( 512, 512 ), Ogre::Vector2( 512, 512 ) );

			Colibri::Window *window = secondaryMgr->createWindow( 0 );
			window->setSize( Ogre::Vector2( 512, 512 ) );

			Colibri::Button *button = secondaryMgr->createWidget<Colibri::Button>( window );
			button->getLabel()->setText( "Hello from Offscreen" );
			button->sizeToFit();

			offscrCanvas->updateCanvas( 0.0f );

			delete offscrCanvas;
		@endcode
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

		/** Creates a texture for the canvas. Will destroy existing one if it exists.
		@param width
		@param height
		@param pixelFormat
		*/
		void createTexture( uint32_t width, uint32_t height,
							Ogre::PixelFormatGpu pixelFormat = Ogre::PFG_RGBA8_UNORM_SRGB );

		/** Returns and releases m_canvasTexture. We no longer own it, and IT BECOMES THE CALLER'S
			RESPONSABILITY TO DESTROY IT after you're done with it.

			This is useful if you want to draw something with Colibri and then use it as a
			static texture. For example, 3D text drawing.
		@param bCreateAnotherOne
			True if you want to create another texture with the exact same settings as
			the current one we're disowning.
		@return
			The current canvas texture. Can be nullptr.
			Caller is responsible for destroying it.
		 */
		Ogre::TextureGpu *colibri_nullable disownCanvasTexture( const bool bCreateAnotherOne );

		Ogre::TextureGpu *colibri_nullable getCanvasTexture() { return m_canvasTexture; }

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

		/** Sets an externally-created workspace. Useful if you need something more complex that what
			createWorkspace() generates for you.

			The workspace should not auto-update.

			Example:
			@code
				ColibriManager *oldValue = colibriCompositorProvider->getColibriManager();
				colibriCompositorProvider->_setColibriManager( m_secondaryManager );

				newWorkspace =
					compositorManager->addWorkspace( sceneManager(),
													 offScreenCanvas->getCanvasTexture(), camera,
													"My Workspace", false );

				colibriCompositorProvider->_setColibriManager( oldValue );

				offScreenCanvas->setWorkspace( newWorkspace, bDestroyCurrent );
			@endcode

		@remarks
			OffScreenCanvas will take ownership of the Workspace, which means it will eventually destroy
			the workspace (i.e. whenever destroyWorkspace() gets called).

			If you don't want that, make sure to call `offScreenCanvas->setWorkspace( nullptr, false );`
			before that happens.
		@param workspace
			New workspace to set.
			Can be nullptr if you want to unset.
		@param bDestroyCurrent
			True if you want to destroy the current workspace.
			False if you don't want to destroy it (we lose all references to that workspace, so
			make sure you still have a reference to it or else it will leak).
		*/
		void setWorkspace( Ogre::CompositorWorkspace *colibri_nullable workspace,
						   bool                                        bDestroyCurrent = true );

		Ogre::CompositorWorkspace *colibri_nullable getWorkspace() { return m_workspace; }

		/** Updates and renders the off-screen canvas.
		@param timeSinceLast
			Time in seconds since last call. Used for animations (if any) as well as anything
			else that requires time (like time stroke repeats).
		*/
		void updateCanvas( const float timeSinceLast );
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
