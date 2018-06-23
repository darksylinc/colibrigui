
#include "ColibriGuiGameState.h"
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

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriWindow.h"
#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriCheckbox.h"
#include "ColibriGui/ColibriEditbox.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriSpinner.h"

using namespace Demo;

namespace Demo
{
	extern Colibri::ColibriManager *colibriManager;
	Colibri::ColibriManager *colibriManager = 0;
	Colibri::Window *mainWindow = 0;
	Colibri::Button *button0 = 0;
	Colibri::Button *button1 = 0;
	Colibri::Spinner *spinner0 = 0;
	Colibri::Checkbox *checkbox0 = 0;
	Colibri::Editbox *editbox0 = 0;

    ColibriGuiGameState::ColibriGuiGameState( const Ogre::String &helpDescription ) :
		TutorialGameState( helpDescription )
	{
    }
    //-----------------------------------------------------------------------------------
    void ColibriGuiGameState::createScene01(void)
    {
        mCameraController = new CameraController( mGraphicsSystem, false );

		//colibriManager = new Colibri::ColibriManager();
		colibriManager->setOgre( mGraphicsSystem->getRoot(),
								 mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager(),
								 mGraphicsSystem->getSceneManager() );
		colibriManager->loadSkins( (mGraphicsSystem->getResourcePath() +
								   "Materials/ColibriGui/Skins.colibri.json").c_str() );

		mainWindow = colibriManager->createWindow( 0 );

		Ogre::Hlms *hlms = mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );
		//mainWindow->setDatablock( hlms->getDefaultDatablock() );

		mainWindow->setTransform( Ogre::Vector2( 0.5, 0.0 ), Ogre::Vector2( 0.5, 0.5 ) );
		mainWindow->setClipBordersMatchSkin();

		/*button0 = colibriManager->createWidget<Colibri::Button>( mainWindow );
		button0->setTopLeft( Ogre::Vector2( 0.0, 0.0 ) );
		button0->setSize( Ogre::Vector2( 0.25, 0.25 ) );

		Colibri::Label *label = colibriManager->createWidget<Colibri::Label>( mainWindow );
		label->setText( "||\n||" );
		label->setTextVertAlignment( Colibri::TextVertAlignment::Center );
		label->setVertReadingDir( Colibri::VertReadingDir::ForceTTB );
		label->sizeToFit( Colibri::States::Idle );
		button0->setSize( label->getSize() );*/

		button0 = colibriManager->createWidget<Colibri::Button>( mainWindow );
		button0->setSkinPack( "ButtonSkin" );
		button0->setTopLeft( Ogre::Vector2( 0.1, 0.21 ) );
		button0->setSize( Ogre::Vector2( 0.25, 0.25 ) );
		button0->getLabel()->setText( "Button 0" );

		for( int i=0 ;i<1; ++i )
		{
			button1 = colibriManager->createWidget<Colibri::Button>( mainWindow );
			button1->setSkinPack( "ButtonSkin" );
			button1->setTopLeft( Ogre::Vector2( 0.1, 0.1 + 0.1 + 0.25 + i * 0.25 ) );
			button1->setSize( Ogre::Vector2( 0.25, 0.25 ) );
			button1->getLabel()->setText( "Button 1" );

			button1->setState( Colibri::States::Disabled );
		}

		button1 = colibriManager->createWidget<Colibri::Button>( mainWindow );
		button1->setSkinPack( "ButtonSkin" );
		button1->setTopLeft( Ogre::Vector2( 0.1 + 0.25, 0.1 + 0.1 + 0.25 + 0.1 ) );
		button1->setSize( Ogre::Vector2( 0.25, 0.25 ) );

		/*spinner0 = colibriManager->createWidget<Colibri::Spinner>( mainWindow );
		spinner0->setSkinPack( "ButtonSkin" );
		spinner0->setTopLeft( Ogre::Vector2::ZERO );
		spinner0->setSize( Ogre::Vector2( 0.25, 0.25 ) );*/
		/*checkbox0 = colibriManager->createWidget<Colibri::Checkbox>( mainWindow );
		checkbox0->setSkinPack( "ButtonSkin" );
		checkbox0->setTopLeft( Ogre::Vector2::ZERO );
		checkbox0->setSize( Ogre::Vector2( 0.25, 0.25 ) );*/
		editbox0 = colibriManager->createWidget<Colibri::Editbox>( mainWindow );
		editbox0->setSkinPack( "ButtonSkin" );
		editbox0->setTopLeft( Ogre::Vector2::ZERO );
		editbox0->setSize( Ogre::Vector2( 0.25, 0.25 ) );

#if 0
		Colibri::Label *label = colibriManager->createWidget<Colibri::Label>( mainWindow );
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
		//label->setTextHorizAlignment( Colibri::TextHorizAlignment::Left );
//		label->setTextVertAlignment( Colibri::TextVertAlignment::Center );
		//label->setTextHorizAlignment( Colibri::TextHorizAlignment::Left );
		//label->setText( "The path of the righteous man is beset on all sides by" );
		label->setSize( mainWindow->getSizeAfterClipping() );
		//label->setVertReadingDir( Colibri::VertReadingDir::ForceTTBLTR );
//		label->sizeToFit( Colibri::States::Idle, 0.5f,
//						  Colibri::TextHorizAlignment::Center, Colibri::TextVertAlignment::Center );
		label->setShadowOutline( true, Ogre::ColourValue::Black, Ogre::Vector2( 1.0f ) );
#endif
		mainWindow->sizeScrollToFit();

        TutorialGameState::createScene01();
    }
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::destroyScene()
	{
		colibriManager->destroyWindow( mainWindow );
		delete colibriManager;
	}
    //-----------------------------------------------------------------------------------
    void ColibriGuiGameState::update( float timeSinceLast )
	{
		static bool tried = false;
		if( !tried )
		{
			SdlInputHandler *inputHandler = mGraphicsSystem->getInputHandler();
			inputHandler->setGrabMousePointer( false );
			inputHandler->setMouseVisible( true );
			tried = true;
		}

		colibriManager->update( timeSinceLast );
		editbox0->update();

		/*static float angle = 0;
		Ogre::Matrix3 rotMat;
		rotMat.FromEulerAnglesXYZ( Ogre::Degree( 0 ), Ogre::Radian( 0 ), Ogre::Radian( angle ) );
		button->setOrientation( rotMat );
		angle += timeSinceLast;*/

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void ColibriGuiGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
	{
		TutorialGameState::generateDebugText( timeSinceLast, outText );
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::mouseMoved( const SDL_Event &arg )
	{
		float width  = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
		float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

		if( arg.type == SDL_MOUSEMOTION )
		{
			Ogre::Vector2 mousePos( arg.motion.x / width, arg.motion.y / height );
			colibriManager->setMouseCursorMoved( mousePos * colibriManager->getCanvasSize() );
		}
		else if( arg.type == SDL_MOUSEWHEEL )
		{
			Ogre::Vector2 mouseScroll( 0.0f, -arg.wheel.y );
			colibriManager->setScroll( mouseScroll * 50.0f * colibriManager->getPixelSize() );
		}

		TutorialGameState::mouseMoved( arg );
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
	{
		float width  = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
		float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

		Ogre::Vector2 mousePos( arg.x / width, arg.y / height );
		colibriManager->setMouseCursorMoved( mousePos * colibriManager->getCanvasSize() );
		colibriManager->setMouseCursorPressed( true, false );
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
	{
		colibriManager->setMouseCursorReleased();
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::keyPressed( const SDL_KeyboardEvent &arg )
	{
		if( arg.keysym.sym == SDLK_w || arg.keysym.sym == SDLK_UP )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Top );
		else if( arg.keysym.sym == SDLK_s || arg.keysym.sym == SDLK_DOWN )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Bottom );
		else if( arg.keysym.sym == SDLK_a || arg.keysym.sym == SDLK_LEFT )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Left );
		else if( arg.keysym.sym == SDLK_d || arg.keysym.sym == SDLK_RIGHT )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Right );
		else if( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ||
			arg.keysym.sym == SDLK_SPACE )
		{
			colibriManager->setKeyboardPrimaryPressed();
		}
	}
    //-----------------------------------------------------------------------------------
    void ColibriGuiGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

		if( arg.keysym.sym == SDLK_w || arg.keysym.sym == SDLK_UP )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Top );
		else if( arg.keysym.sym == SDLK_s || arg.keysym.sym == SDLK_DOWN )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Bottom );
		else if( arg.keysym.sym == SDLK_a || arg.keysym.sym == SDLK_LEFT )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Left );
		else if( arg.keysym.sym == SDLK_d || arg.keysym.sym == SDLK_RIGHT )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Right );
		else if( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ||
				 arg.keysym.sym == SDLK_SPACE )
		{
			colibriManager->setKeyboardPrimaryReleased();
		}

		TutorialGameState::keyReleased( arg );
    }
}
