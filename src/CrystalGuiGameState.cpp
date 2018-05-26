
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

#include "OgreLogManager.h"

#include "CrystalGui/CrystalManager.h"
#include "CrystalGui/CrystalWindow.h"
#include "CrystalGui/CrystalButton.h"
#include "CrystalGui/CrystalLabel.h"

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
		mainWindow->setClipBordersMatchSkin();

		button0 = crystalManager->createWidget<Crystal::Button>( mainWindow );
		button0->setSkinPack( "ButtonSkin" );
		button0->setTopLeft( Ogre::Vector2( 0.1, 0.1 ) );
		button0->setSize( Ogre::Vector2( 0.25, 0.25 ) );

		button1 = crystalManager->createWidget<Crystal::Button>( mainWindow );
		button1->setSkinPack( "ButtonSkin" );
		button1->setTopLeft( Ogre::Vector2( 0.1, 0.1 + 0.1 + 0.25 ) );
		button1->setSize( Ogre::Vector2( 0.25, 0.25 ) );

		button1 = crystalManager->createWidget<Crystal::Button>( mainWindow );
		button1->setSkinPack( "ButtonSkin" );
		button1->setTopLeft( Ogre::Vector2( 0.1 + 0.25, 0.1 + 0.1 + 0.25 + 0.1 ) );
		button1->setSize( Ogre::Vector2( 0.25, 0.25 ) );

#if 1
		Crystal::Label *label = crystalManager->createWidget<Crystal::Label>( mainWindow );
		label->setText( "The path of the righteous man is beset on all sides by the iniquities\n"
						"of the selfish and the tyranny of evil men. Blessed is he who, in the\n"
						"name of charity and good will, shepherds the weak through the valley \n"
						"of darkness, for he is truly his brother's keeper and the finder of \n"
						"lost children. And I will strike down upon thee with great vengeance \n"
						"and furious anger those who would attempt to poison and destroy My \n"
						"brothers. And you will know My name is the Lord when I lay My \n"
						"vengeance upon thee." );
		/*label->setText( "The path of the righteous man is beset on all sides by the iniquities "
						"of the selfish and the tyranny of evil men. Blessed is he who, in the "
						"name of charity and good will, shepherds the weak through the valley "
						"of darkness, for he is truly his brother's keeper and the finder of "
						"lost children. And I will strike down upon thee with great vengeance "
						"and furious anger those who would attempt to poison and destroy My "
						"brothers. And you will know My name is the Lord when I lay My "
						"vengeance upon thee." );*/
//		label->setText( "The path of the righteous man is beset on all sides by the iniquities "
//						"of the selfish and the tyranny of evil men. Blessed is he who, in the" );
		//label->setText( "The path of the righteous man is beset on all sides by the iniquities" );
		//label->setText( "\tHola qué tal?\t\tHola2\n  Hola qué tal?\tHola2\n" );
		//label->setText( "H\nH" );
		//label->setText( "\tHola\t\tQ\nQue Tal\nHola2\nHola3" );
		//label->setText( "懶惰的姜貓懶惰的姜貓懶惰97\t的姜貓懶惰的姜貓懶惰的姜貓Hola\n懶惰的姜貓Hello\n懶惰的姜貓" );
		//label->setText( "H\tH" );
		//label->setText( "aaa\naaa\n\naga\nagaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" );
		//label->setText( "Hello\t\tTest\nHello2\t\tTest" );
		//label->setText( "\tHello\n    asd" );
		//label->setText( "من أنا لاستجواب أولئك الذين يكتبونh\nola" );
//		label->setText( "من أنا لاستجواب ""\n\r"
//						"أولئك الذين يكتبونhola" );
		{
			/*std::ifstream file( "/home/matias/Desktop/Text2", std::ios::in|std::ios::binary );
			file.seekg( 0, std::ios::end );
			const size_t fileSize = file.tellg();
			file.seekg( 0, std::ios::beg );
			std::string text;
			text.resize( fileSize );
			file.read( &text[0], fileSize );*/
			//label->setText( text );
			//label->setText( "Hola\nQue tal sin paragraph?" );
			//label->setText( "Hola\u2029\nQue tal?" );
			//label->setText( "تجوستجوستجThisتجوستجوستج is a ستجو word ستجوستجوستجوستج" );
			//label->setText( "الذي يحيي ذكرى احتجاجات مواطنين فلسطينيين من مدن" );
		}
		//label->setTextHorizAlignment( Crystal::TextHorizAlignment::Left );
//		label->setTextVertAlignment( Crystal::TextVertAlignment::Center );
		//label->setTextHorizAlignment( Crystal::TextHorizAlignment::Left );
		//label->setText( "The path of the righteous man is beset on all sides by" );
		label->setSize( mainWindow->getSizeAfterClipping() );
		//label->setVertReadingDir( Crystal::VertReadingDir::ForceTTBLTR );
//		label->sizeToFit( Crystal::States::Idle, 0.5f,
//						  Crystal::TextHorizAlignment::Center, Crystal::TextVertAlignment::Center );
		label->setShadowOutline( true, Ogre::ColourValue::Black, Ogre::Vector2( 1.0f ) );

#endif
		mainWindow->sizeScrollToFit();

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

		crystalManager->update( timeSinceLast );

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
		if( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ||
			arg.keysym.sym == SDLK_SPACE )
		{
			crystalManager->setKeyboardPrimaryPressed();
		}
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
		else if( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ||
				 arg.keysym.sym == SDLK_SPACE )
		{
			crystalManager->setKeyboardPrimaryReleased();
		}

		TutorialGameState::keyReleased( arg );
    }
}
