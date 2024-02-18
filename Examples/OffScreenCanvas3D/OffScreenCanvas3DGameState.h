
#ifndef _Demo_OffScreenCanvas3DGameState_H_
#define _Demo_OffScreenCanvas3DGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "ColibriGui/ColibriGuiPrerequisites.h"

namespace Demo
{
	static constexpr size_t kNum3DTexts = 3u;

	class OffScreenCanvas3DGameState final : public TutorialGameState
	{
		Ogre::Rectangle2D *mText3D[kNum3DTexts];
		Ogre::SceneNode *mNodes[kNum3DTexts];

		Colibri::OffScreenCanvas *mOffscreenCanvas;

		Colibri::Label *mLabelOffscreen;

		Colibri::ColibriManager *getColibriManager();

		float mAccumTime;

		void drawTextIn3D( Ogre::Rectangle2D *rectangle, const char *text,
						   float scaleInWorldUnits = 1.0f );

		void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

	public:
		OffScreenCanvas3DGameState( const Ogre::String &helpDescription );

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
