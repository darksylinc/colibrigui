
#include "CrystalGuiGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsTextureManager.h"
#include "OgreHlmsPbs.h"

#include "OgreWindow.h"
#include "SdlInputHandler.h"

#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/CrystalWindow.h"
#include "CrystalGui/CrystalButton.h"

using namespace Demo;

namespace Demo
{
	extern Crystal::CrystalManager *crystalManager;
	Crystal::CrystalManager *crystalManager = 0;
	Crystal::Window *mainWindow = 0;
	Crystal::Button *button0 = 0;
	Crystal::Button *button1 = 0;

    CrystalGuiGameState::CrystalGuiGameState( const Ogre::String &helpDescription ) :
		TutorialGameState( helpDescription )
	{
    }
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::createScene01(void)
    {
        mCameraController = new CameraController( mGraphicsSystem, false );

		//crystalManager = new Crystal::CrystalManager();
		crystalManager->setOgre( mGraphicsSystem->getRoot(),
								 mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager(),
								 mGraphicsSystem->getSceneManager() );
		crystalManager->loadSkins( (mGraphicsSystem->getResourcePath() +
								   "Materials/CrystalGui/Skins.crystal.json").c_str() );

		mainWindow = crystalManager->createWindow( 0 );

		Ogre::Hlms *hlms = mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );
		//mainWindow->setDatablock( hlms->getDefaultDatablock() );
		mainWindow->setSkinPack( "ButtonSkin" );

		mainWindow->setTransform( Ogre::Vector2( 0.5, 0.0 ), Ogre::Vector2( 0.5, 0.5 ) );

		button0 = crystalManager->createWidget<Crystal::Button>( mainWindow );
		button0->setSkinPack( "ButtonSkin" );
		button0->setTopLeft( Ogre::Vector2( 0.1, 0.1 ) );
		button0->setSize( Ogre::Vector2( 0.25, 0.25 ) );

		button1 = crystalManager->createWidget<Crystal::Button>( mainWindow );
		button1->setSkinPack( "ButtonSkin" );
		button1->setTopLeft( Ogre::Vector2( 0.1, 0.1 + 0.1 + 0.25 ) );
		button1->setSize( Ogre::Vector2( 0.25, 0.25 ) );

		mGraphicsSystem->getSceneManager()->getRootSceneNode()->attachObject( mainWindow );
		mGraphicsSystem->getSceneManager()->getRootSceneNode()->attachObject( button0 );
		mGraphicsSystem->getSceneManager()->getRootSceneNode()->attachObject( button1 );

        TutorialGameState::createScene01();
    }
	//-----------------------------------------------------------------------------------
	void CrystalGuiGameState::destroyScene()
	{
		crystalManager->destroyWindow( mainWindow );
		delete crystalManager;
	}
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::update( float timeSinceLast )
	{
		static bool tried = false;
		if( !tried )
		{
			SdlInputHandler *inputHandler = mGraphicsSystem->getInputHandler();
			inputHandler->setGrabMousePointer( false );
			inputHandler->setMouseVisible( true );
			tried = true;
		}

		crystalManager->update();

		/*static float angle = 0;
		Ogre::Matrix3 rotMat;
		rotMat.FromEulerAnglesXYZ( Ogre::Degree( 0 ), Ogre::Radian( 0 ), Ogre::Radian( angle ) );
		button->setOrientation( rotMat );
		angle += timeSinceLast;*/

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
	{
		TutorialGameState::generateDebugText( timeSinceLast, outText );
	}
	//-----------------------------------------------------------------------------------
	void CrystalGuiGameState::mouseMoved( const SDL_Event &arg )
	{
		float width  = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
		float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

		Ogre::Vector2 mousePos( arg.motion.x / width, arg.motion.y / height );
		crystalManager->setMouseCursorMoved( mousePos * crystalManager->getCanvasSize() );
		TutorialGameState::mouseMoved( arg );
	}
	//-----------------------------------------------------------------------------------
	void CrystalGuiGameState::mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
	{
		float width  = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
		float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

		Ogre::Vector2 mousePos( arg.x / width, arg.y / height );
		crystalManager->setMouseCursorMoved( mousePos * crystalManager->getCanvasSize() );
		crystalManager->setMouseCursorPressed();
	}
	//-----------------------------------------------------------------------------------
	void CrystalGuiGameState::mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
	{
		crystalManager->setMouseCursorReleased();
	}
	//-----------------------------------------------------------------------------------
	void CrystalGuiGameState::keyPressed( const SDL_KeyboardEvent &arg )
	{
		if( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER )
			crystalManager->setMouseCursorPressed();
	}
    //-----------------------------------------------------------------------------------
    void CrystalGuiGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

		if( arg.keysym.sym == SDLK_w || arg.keysym.sym == SDLK_UP )
			crystalManager->setKeyDirection( Crystal::Borders::Top );
		else if( arg.keysym.sym == SDLK_s || arg.keysym.sym == SDLK_DOWN )
			crystalManager->setKeyDirection( Crystal::Borders::Bottom );
		else if( arg.keysym.sym == SDLK_a || arg.keysym.sym == SDLK_LEFT )
			crystalManager->setKeyDirection( Crystal::Borders::Left );
		else if( arg.keysym.sym == SDLK_d || arg.keysym.sym == SDLK_RIGHT )
			crystalManager->setKeyDirection( Crystal::Borders::Right );
		else if( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER )
			crystalManager->setMouseCursorReleased();

		TutorialGameState::keyReleased( arg );
    }
}
