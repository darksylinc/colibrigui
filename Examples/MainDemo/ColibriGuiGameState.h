
#ifndef _Demo_ColibriGuiGameState_H_
#define _Demo_ColibriGuiGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Colibri
{
	class ColibriManager;
}

namespace Demo
{
	class ColibriGuiGameState final : public TutorialGameState
	{
		void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

		Colibri::ColibriManager *getColibriManager();

	public:
		ColibriGuiGameState( const Ogre::String &helpDescription );

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
