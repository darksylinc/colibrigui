
#ifndef _Demo_CrystalGuiGameState_H_
#define _Demo_CrystalGuiGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class CrystalGuiGameState : public TutorialGameState
    {
        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        CrystalGuiGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
		virtual void destroyScene();

        virtual void update( float timeSinceLast );

		virtual void mouseMoved( const SDL_Event &arg );
		virtual void mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id );
		virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id );
		virtual void keyPressed( const SDL_KeyboardEvent &arg );
        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
