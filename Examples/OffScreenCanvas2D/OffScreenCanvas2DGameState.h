
#ifndef _Demo_OffScreenCanvas2DGameState_H_
#define _Demo_OffScreenCanvas2DGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "ColibriGui/ColibriGuiPrerequisites.h"

namespace Demo
{
	static constexpr size_t kNum3DTexts = 3u;

	class OffScreenCanvas2DGameState final : public TutorialGameState
	{
		Colibri::OffScreenCanvas *mOffscreenCanvas;
		Colibri::Window *mOffscreenRootWindow;

		Colibri::ColibriManager *getColibriManager();

		void setCustomTextureToSkin( Colibri::Renderable *widget, Ogre::TextureGpu *texture,
									 const bool bTransparent );
		void destroyLinkedResources( Ogre::TextureGpu *texture );

		void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

	public:
		OffScreenCanvas2DGameState( const Ogre::String &helpDescription );

		void createScene01() override;
		void destroyScene() override;

		void update( float timeSinceLast ) override;

		void mouseMoved( const SDL_Event &arg ) override;
		void mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id ) override;
		void mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id ) override;
		void textEditing( const SDL_TextEditingEvent &arg ) override;
		void textInput( const SDL_TextInputEvent &arg ) override;
		void keyPressed( const SDL_KeyboardEvent &arg ) override;
		void keyReleased( const SDL_KeyboardEvent &arg ) override;
	};
}  // namespace Demo

#endif
