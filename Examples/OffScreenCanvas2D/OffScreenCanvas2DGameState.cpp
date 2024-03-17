
#include "OffScreenCanvas2DGameState.h"

#include "CameraController.h"
#include "ExamplesCommon.h"

#include "OgreCamera.h"
#include "OgreFrameStats.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRectangle2D2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"
#include "OgreWindow.h"

#include "SdlInputHandler.h"

#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriOffScreenCanvas.h"
#include "ColibriGui/ColibriSkinManager.h"
#include "ColibriGui/ColibriWindow.h"
#include "ColibriGui/Text/ColibriBmpFont.h"
#include "ColibriGui/Text/ColibriShaperManager.h"

using namespace Demo;

static Colibri::Window *fullWindow = 0;

OffScreenCanvas2DGameState::OffScreenCanvas2DGameState( const Ogre::String &helpDescription ) :
	TutorialGameState( helpDescription ),
	mOffscreenCanvas( 0 ),
	mOffscreenRootWindow( 0 )
{
}
//-----------------------------------------------------------------------------------
Colibri::ColibriManager *OffScreenCanvas2DGameState::getColibriManager()
{
	COLIBRI_ASSERT_HIGH( dynamic_cast<ColibriGuiGraphicsSystem *>( mGraphicsSystem ) );
	return static_cast<ColibriGuiGraphicsSystem *>( mGraphicsSystem )->getColibriManager();
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::setCustomTextureToSkin( Colibri::Renderable *widget,
														 Ogre::TextureGpu *texture,
														 const bool bTransparent )
{
	Ogre::Hlms *hlmsUnlit = mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );

	// Create a material with the same name as the texture.
	// The name doesn't necessarily have to be the same as the texture.
	const Ogre::String materialName = texture->getNameStr();

	Ogre::HlmsDatablock *datablock = hlmsUnlit->getDatablock( materialName );

	if( !datablock )
	{
		using namespace Ogre;
		// First time. Create it.
		HlmsParamVec params;
		params.push_back( { "diffuse_map", mOffscreenCanvas->getCanvasTexture()->getNameStr() } );

		HlmsMacroblock macroblock;
		macroblock.mDepthCheck = false;
		macroblock.mDepthWrite = false;
		HlmsBlendblock blendblock;
		if( bTransparent )
			blendblock.setBlendType( Ogre::SBT_TRANSPARENT_ALPHA );
		datablock =
			hlmsUnlit->createDatablock( materialName, materialName, macroblock, blendblock, params );
	}

	Colibri::ColibriManager *colibriManager = getColibriManager();
	Colibri::SkinManager *skinManager = colibriManager->getSkinManager();

	const Colibri::SkinInfoMap &skins = skinManager->getSkins();
	Colibri::SkinInfoMap::const_iterator itor = skins.find( "EmptyBg" );

	if( itor != skins.end() )
	{
		Colibri::SkinInfo *skinInfos[Colibri::States::NumStates];
		Colibri::SkinInfo mainSkinInfo;

		mainSkinInfo = itor->second;
		mainSkinInfo.materialName = materialName;
		mainSkinInfo.stateInfo.materialName = materialName;

		mainSkinInfo.stateInfo.uvTopLeftBottomRight[Colibri::GridLocations::Center] =
			Ogre::Vector4( 0.0f, 0.0f, 1.0f, 1.0f );

		for( size_t i = 0; i < Colibri::States::NumStates; ++i )
			skinInfos[i] = &mainSkinInfo;
		widget->_setSkinPack( skinInfos );
	}
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::destroyLinkedResources( Ogre::TextureGpu *texture )
{
	// We take the advantage that in setCustomTextureToSkin we created the
	// texture and material with the same name.
	Ogre::Hlms *hlmsUnlit = mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );
	const Ogre::String materialName = texture->getNameStr();
	try
	{
		hlmsUnlit->destroyDatablock( materialName );
	}
	catch( Ogre::ItemIdentityException &e )
	{
		// We never called setCustomTextureToSkin(). Ignore it, but log
		// this happened since it may indicate wrong setup.
		Ogre::LogManager::getSingleton().logMessage( "WARNING: " + e.getFullDescription() );
	}
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::createScene01( void )
{
	// mCameraController = new CameraController( mGraphicsSystem, false );

	Ogre::Window *window = mGraphicsSystem->getRenderWindow();
	Colibri::ColibriManager *colibriManager = getColibriManager();

	const float aspectRatioColibri =
		static_cast<float>( window->getHeight() ) / static_cast<float>( window->getWidth() );
	colibriManager->setCanvasSize( Ogre::Vector2( 1920.0f, 1920.0f * aspectRatioColibri ),
								   Ogre::Vector2( window->getWidth(), window->getHeight() ) );

	// colibriManager = new Colibri::ColibriManager();
	colibriManager->setOgre( mGraphicsSystem->getRoot(),
							 mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager(),
							 mGraphicsSystem->getSceneManager() );
	colibriManager->loadSkins( ( mGraphicsSystem->getResourcePath() +
								 "Materials/ColibriGui/Skins/DarkGloss/Skins.colibri.json" )
								   .c_str() );

	// colibriManager->setTouchOnlyMode( true );

	fullWindow = colibriManager->createWindow( 0 );
	fullWindow->setSize( colibriManager->getCanvasSize() );

	fullWindow->setDebugName( "Main fullWindow" );
	// When m_breadthFirst is set to true, it can cause significant performance
	// increases for UI-heavy applications. But be sure you understand it i.e.
	// it may not render correctly if your widgets have children and they overlap.
	fullWindow->m_breadthFirst = true;

	fullWindow->setSize( colibriManager->getCanvasSize() );
	fullWindow->setSkin( "EmptyBg" );
	fullWindow->setVisualsEnabled( false );
	{
		Colibri::Label *helpText = colibriManager->createWidget<Colibri::Label>( fullWindow );
		helpText->setText(
			"This sample draws the UI into a secondary ColibriManager and bakes\n"
			"the rendered result into a texture, updated every frame.\n"
			"The result is shown in a UI widget in the primary ColibriManager.\n"
			"\n"
			"The offscreen UI cannot be interacted in this demo with simply because the Mouse/Keyboard\n"
			"events are not being forwarded to mOffscreenCanvas->getSecondaryManager()." );
		helpText->sizeToFit();
		helpText->setTopLeft( fullWindow->getSize() - helpText->getSize() );
	}

	{
		mOffscreenCanvas = new Colibri::OffScreenCanvas( colibriManager );

		Colibri::ColibriManager *secondaryManager = mOffscreenCanvas->getSecondaryManager();
		secondaryManager->setCanvasSize( Ogre::Vector2( 512.0f ), Ogre::Vector2( 512.0f ) );

		mOffscreenRootWindow = secondaryManager->createWindow( 0 );
		mOffscreenRootWindow->setDebugName( "Offscreen Root Window" );
		mOffscreenRootWindow->m_breadthFirst = true;

		mOffscreenRootWindow->setSize( Ogre::Vector2( 512.0f ) );
		Colibri::Button *button =
			secondaryManager->createWidget<Colibri::Button>( mOffscreenRootWindow );
		button->getLabel()->setText( "Offscreen Button" );
		button->sizeToFit();

		mOffscreenCanvas->createTexture( 512u, 512u );
		mOffscreenCanvas->createWorkspace(
			static_cast<ColibriGuiGraphicsSystem *>( mGraphicsSystem )->getColibriCompoProvider(),
			mGraphicsSystem->getCamera() );
	}

	Colibri::Renderable *viewToOffscreen =
		colibriManager->createWidget<Colibri::Renderable>( fullWindow );
	viewToOffscreen->setSize( Ogre::Vector2( 512.0f ) );
	setCustomTextureToSkin( viewToOffscreen, mOffscreenCanvas->getCanvasTexture(), false );

	TutorialGameState::createScene01();
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::destroyScene()
{
	// fullWindow MUST be destroyed first, because it contains the Widget with the
	// material & texture that we'll destroy in destroyLinkedResources().
	Colibri::ColibriManager *colibriManager = getColibriManager();
	colibriManager->destroyWindow( fullWindow );

	if( mOffscreenCanvas )
	{
		mOffscreenCanvas->getSecondaryManager()->destroyWindow( mOffscreenRootWindow );
		mOffscreenRootWindow = 0;
		// Because we never called mOffscreenCanvas->disownCanvasTexture, mOffscreenCanvas owns
		// the TextureGpu and it will be destroyed when we do `delete mOffscreenCanvas`.
		destroyLinkedResources( mOffscreenCanvas->getCanvasTexture() );

		delete mOffscreenCanvas;  // We MUST delete the OffScreenCanvas before the main ColibriManager.
		mOffscreenCanvas = 0;
	}
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::update( float timeSinceLast )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();

	static bool tried = false;
	if( !tried )
	{
		SdlInputHandler *inputHandler = mGraphicsSystem->getInputHandler();
		inputHandler->setGrabMousePointer( false );
		inputHandler->setMouseVisible( true );
		inputHandler->setMouseRelative( false );
		tried = true;
	}

	colibriManager->update( timeSinceLast );

	const bool isTextInputActive = SDL_IsTextInputActive();

	if( colibriManager->focusedWantsTextInput() && !isTextInputActive )
		SDL_StartTextInput();
	else if( !colibriManager->focusedWantsTextInput() && isTextInputActive )
		SDL_StopTextInput();

	if( isTextInputActive )
	{
		static SDL_Rect oldRect = { 0, 0, 0, 0 };
		// We tried to update this only inside OffScreenCanvas2DGameState::keyPressed
		// but it didn't work well in Linux with fcitx
		Ogre::Vector2 imeOffset = colibriManager->getImeLocation();
		SDL_Rect rect;
		rect.x = imeOffset.x;
		rect.y = imeOffset.y;
		rect.w = 0;
		rect.h = 0;
		if( oldRect.x != rect.x || oldRect.y != rect.y )
			SDL_SetTextInputRect( &rect );
	}

	mOffscreenCanvas->updateCanvas( timeSinceLast );

	TutorialGameState::update( timeSinceLast );
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
{
	TutorialGameState::generateDebugText( timeSinceLast, outText );
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::mouseMoved( const SDL_Event &arg )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();

	float width = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
	float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

	if( arg.type == SDL_MOUSEMOTION )
	{
		Ogre::Vector2 mousePos( arg.motion.x / width, arg.motion.y / height );
		colibriManager->setMouseCursorMoved( mousePos * colibriManager->getCanvasSize() );
	}
	else if( arg.type == SDL_MOUSEWHEEL )
	{
		Ogre::Vector2 mouseScroll( arg.wheel.x, -arg.wheel.y );
		colibriManager->setScroll( mouseScroll * 50.0f * colibriManager->getCanvasSize() *
								   colibriManager->getPixelSize() );
	}

	TutorialGameState::mouseMoved( arg );
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();

	float width = static_cast<float>( mGraphicsSystem->getRenderWindow()->getWidth() );
	float height = static_cast<float>( mGraphicsSystem->getRenderWindow()->getHeight() );

	Ogre::Vector2 mousePos( arg.x / width, arg.y / height );
	colibriManager->setMouseCursorMoved( mousePos * colibriManager->getCanvasSize() );
	colibriManager->setMouseCursorPressed( true, false );
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();
	colibriManager->setMouseCursorReleased();
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::textEditing( const SDL_TextEditingEvent &arg )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();
	colibriManager->setTextEdit( arg.text, arg.start, arg.length );
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::textInput( const SDL_TextInputEvent &arg )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();
	colibriManager->setTextInput( arg.text, false );
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::keyPressed( const SDL_KeyboardEvent &arg )
{
	Colibri::ColibriManager *colibriManager = getColibriManager();

	const bool isTextInputActive = SDL_IsTextInputActive();
	const bool isTextMultiline = colibriManager->isTextMultiline();

	if( ( arg.keysym.sym == SDLK_w && !isTextInputActive ) || arg.keysym.sym == SDLK_UP )
		colibriManager->setKeyDirectionPressed( Colibri::Borders::Top );
	else if( ( arg.keysym.sym == SDLK_s && !isTextInputActive ) || arg.keysym.sym == SDLK_DOWN )
		colibriManager->setKeyDirectionPressed( Colibri::Borders::Bottom );
	else if( ( arg.keysym.sym == SDLK_a && !isTextInputActive ) || arg.keysym.sym == SDLK_LEFT )
		colibriManager->setKeyDirectionPressed( Colibri::Borders::Left );
	else if( ( arg.keysym.sym == SDLK_d && !isTextInputActive ) || arg.keysym.sym == SDLK_RIGHT )
		colibriManager->setKeyDirectionPressed( Colibri::Borders::Right );
	else if( ( ( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ) &&
			   !isTextMultiline ) ||
			 ( arg.keysym.sym == SDLK_SPACE && !isTextInputActive ) )
	{
		colibriManager->setKeyboardPrimaryPressed();
	}
	else if( isTextInputActive )
	{
		if( ( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ) && isTextMultiline )
			colibriManager->setTextSpecialKeyPressed( SDLK_RETURN, arg.keysym.mod );
		else
		{
			colibriManager->setTextSpecialKeyPressed( arg.keysym.sym & ~SDLK_SCANCODE_MASK,
													  arg.keysym.mod );
		}
	}
}
//-----------------------------------------------------------------------------------
void OffScreenCanvas2DGameState::keyReleased( const SDL_KeyboardEvent &arg )
{
	if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS ) ) != 0 )
	{
		TutorialGameState::keyReleased( arg );
		return;
	}

	Colibri::ColibriManager *colibriManager = getColibriManager();

	const bool isTextInputActive = SDL_IsTextInputActive();
	const bool isTextMultiline = colibriManager->isTextMultiline();

	if( ( arg.keysym.sym == SDLK_w && !isTextInputActive ) || arg.keysym.sym == SDLK_UP )
		colibriManager->setKeyDirectionReleased( Colibri::Borders::Top );
	else if( ( arg.keysym.sym == SDLK_s && !isTextInputActive ) || arg.keysym.sym == SDLK_DOWN )
		colibriManager->setKeyDirectionReleased( Colibri::Borders::Bottom );
	else if( ( arg.keysym.sym == SDLK_a && !isTextInputActive ) || arg.keysym.sym == SDLK_LEFT )
		colibriManager->setKeyDirectionReleased( Colibri::Borders::Left );
	else if( ( arg.keysym.sym == SDLK_d && !isTextInputActive ) || arg.keysym.sym == SDLK_RIGHT )
		colibriManager->setKeyDirectionReleased( Colibri::Borders::Right );
	else if( ( ( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ) &&
			   !isTextMultiline ) ||
			 ( arg.keysym.sym == SDLK_SPACE && !isTextInputActive ) )
	{
		colibriManager->setKeyboardPrimaryReleased();
	}
	else if( isTextInputActive )
	{
		if( ( arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER ) && isTextMultiline )
			colibriManager->setTextSpecialKeyReleased( SDLK_RETURN, arg.keysym.mod );
		else
		{
			colibriManager->setTextSpecialKeyReleased( arg.keysym.sym & ~SDLK_SCANCODE_MASK,
													   arg.keysym.mod );
		}
	}

	TutorialGameState::keyReleased( arg );
}
