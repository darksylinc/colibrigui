
#include "ColibriGuiGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
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
#include "ColibriGui/ColibriProgressbar.h"
#include "ColibriGui/ColibriSlider.h"

#include "ColibriGui/Layouts/ColibriLayoutLine.h"
#include "ColibriGui/Layouts/ColibriLayoutMultiline.h"
#include "ColibriGui/Layouts/ColibriLayoutTableSameSize.h"

using namespace Demo;

namespace Demo
{
	extern Colibri::ColibriManager *colibriManager;
	Colibri::ColibriManager *colibriManager = 0;
	Colibri::Window *mainWindow = 0;
	Colibri::Window *vertWindow = 0;
	Colibri::Button *button0 = 0;
	Colibri::Button *button1 = 0;
	Colibri::Spinner *spinner0 = 0;
	Colibri::Checkbox *checkbox0 = 0;
	Colibri::Editbox *editbox0 = 0;
	Colibri::Progressbar *progressBar0 = 0;
	Colibri::Progressbar *progressBar1 = 0;
	Colibri::Slider *slider1 = 0;
	Colibri::Slider *slider2 = 0;
	Colibri::Label *sliderLabel = 0;

	Colibri::Window *overlapWindow1 = 0;
	Colibri::Window *overlapWindow2 = 0;
	Colibri::Button *orderButtonBack = 0;
	Colibri::Button *orderButtonFront = 0;
	Colibri::Window *innerOverlapWindow1 = 0;
	Colibri::Window *innerOverlapWindow2 = 0;
	Colibri::Button* innerOverlapButton1 = 0;
	Colibri::Button* innerOverlapButton2 = 0;
	Colibri::Button *innerToggleOrder = 0;

	class DemoWidgetListener : public Colibri::WidgetActionListener
	{
	public:
		virtual ~DemoWidgetListener() {}
		virtual void notifyWidgetAction( Colibri::Widget *widget, Colibri::Action::Action action )
		{
			if( action == Colibri::Action::Action::PrimaryActionPerform )
			{
				if( widget == orderButtonBack )
					overlapWindow1->setZOrder( 3 );
				else if( widget == orderButtonFront )
					overlapWindow1->setZOrder( 5 );
				else if( widget == innerToggleOrder )
				{
					uint8_t oldOrder = innerOverlapWindow1->getZOrder();
					innerOverlapWindow1->setZOrder( innerOverlapWindow2->getZOrder() );
					innerOverlapButton1->setZOrder( innerOverlapWindow2->getZOrder() );
					innerOverlapWindow2->setZOrder( oldOrder );
					innerOverlapButton2->setZOrder( oldOrder );
				}
			}
		}
	};
	DemoWidgetListener* demoActionListener;

	ColibriGuiGameState::ColibriGuiGameState( const Ogre::String &helpDescription ) :
		TutorialGameState( helpDescription )
	{
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::createScene01(void)
	{
		mCameraController = new CameraController( mGraphicsSystem, false );

		Ogre::Window *window = mGraphicsSystem->getRenderWindow();

		colibriManager->setCanvasSize( Ogre::Vector2( 1920.0f, 1080.0f ),
									   Ogre::Vector2( window->getWidth(), window->getHeight() ) );

		//colibriManager = new Colibri::ColibriManager();
		colibriManager->setOgre( mGraphicsSystem->getRoot(),
								 mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager(),
								 mGraphicsSystem->getSceneManager() );
		colibriManager->loadSkins( (mGraphicsSystem->getResourcePath() +
								   "Materials/ColibriGui/Skins/DarkGloss/Skins.colibri.json").c_str() );

		mainWindow = colibriManager->createWindow( 0 );
		//mainWindow->setVisualsEnabled( false );
		vertWindow = colibriManager->createWindow( 0 );

		//Ogre::Hlms *hlms = mGraphicsSystem->getRoot()->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );
		//mainWindow->setDatablock( hlms->getDefaultDatablock() );

		mainWindow->setTransform( Ogre::Vector2( 0, 0 ), Ogre::Vector2( 450, 0 ) );
		vertWindow->setTransform( Ogre::Vector2( colibriManager->getCanvasSize().x - 64, 0 ),
								  Ogre::Vector2( 64, 450 ) );

		//When m_breadthFirst is set to true, it can cause significant performance
		//increases for UI-heavy applications. But be sure you understand it i.e.
		//it may not render correctly if your widgets have children and they overlap.
		mainWindow->m_breadthFirst = true;
		vertWindow->m_breadthFirst = true;

		Colibri::LayoutLine *layout = new Colibri::LayoutLine( colibriManager );
		//layout->addCell( &Colibri::LayoutSpacer::c_DefaultBlankSpacer );

		button0 = colibriManager->createWidget<Colibri::Button>( mainWindow );
		button0->m_minSize = Ogre::Vector2( 350, 64 );
		button0->getLabel()->setText( "This is a button" );
		button0->sizeToFit();
		layout->addCell( button0 );

		checkbox0 = colibriManager->createWidget<Colibri::Checkbox>( mainWindow );
		checkbox0->m_minSize = Ogre::Vector2( 350, 64 );
		checkbox0->getButton()->getLabel()->setText( "This is a checkbox" );
		layout->addCell( checkbox0 );

		checkbox0 = colibriManager->createWidget<Colibri::Checkbox>( mainWindow );
		checkbox0->m_minSize = Ogre::Vector2( 350, 64 );
		checkbox0->setTriState( true );
		checkbox0->getButton()->getLabel()->setText( "This is a tri-state checkbox" );
//		checkbox0->sizeToFit();
//		checkbox0->setSize( checkbox0->getSize() + Ogre::Vector2( 0, 32 ) );
		layout->addCell( checkbox0 );

		checkbox0 = colibriManager->createWidget<Colibri::Checkbox>( mainWindow );
		checkbox0->m_minSize = Ogre::Vector2( 350, 64 );
		checkbox0->getButton()->getLabel()->setText( "This checkbox has the tickmark outside" );
		checkbox0->setCheckboxMode( Colibri::Checkbox::TickButton );
		layout->addCell( checkbox0 );

		spinner0 = colibriManager->createWidget<Colibri::Spinner>( mainWindow );
		spinner0->setTopLeft( Ogre::Vector2::ZERO );
		spinner0->m_minSize = Ogre::Vector2( 350, 64 );
		spinner0->getLabel()->setText( "Options" );
		{
			std::vector<std::string> options;
			options.push_back( "Test" );
			options.push_back( "Low" );
			options.push_back( "Medium" );
			options.push_back( "High" );
			spinner0->setOptions( options );
		}
		layout->addCell( spinner0 );

		spinner0 = colibriManager->createWidget<Colibri::Spinner>( mainWindow );
		spinner0->setTopLeft( Ogre::Vector2::ZERO );
		spinner0->m_minSize = Ogre::Vector2( 350, 64 );
		spinner0->getLabel()->setText( "Numeric Spinner" );
		layout->addCell( spinner0 );

		spinner0 = colibriManager->createWidget<Colibri::Spinner>( mainWindow );
		spinner0->setTopLeft( Ogre::Vector2::ZERO );
		spinner0->m_minSize = Ogre::Vector2( 350, 64 );
		spinner0->setFixedWidth( true, 0 );
		spinner0->setHorizWidgetDir( Colibri::HorizWidgetDir::AutoRTL );
		spinner0->getLabel()->setText( "This spinner is on the other side" );
		layout->addCell( spinner0 );

		editbox0 = colibriManager->createWidget<Colibri::Editbox>( mainWindow );
		editbox0->m_minSize = Ogre::Vector2( 350, 64 );
		editbox0->setText( "You can edit this text" );
		editbox0->m_expand[0] = true;
		layout->addCell( editbox0 );

		progressBar0 = colibriManager->createWidget<Colibri::Progressbar>( mainWindow );
		progressBar0->m_minSize = Ogre::Vector2( 350, 64 );
		progressBar0->setProgress( 0.75f );
		layout->addCell( progressBar0 );

		progressBar1 = colibriManager->createWidget<Colibri::Progressbar>( mainWindow );
		progressBar1->m_minSize = Ogre::Vector2( 350, 64 );
		progressBar1->setProgress( 0.75f );
		progressBar1->setVertical( true );
		progressBar1->getProgressLayer()->setColour( true, Ogre::ColourValue( 0.0f, 0.7f, 0.2f ) );
		layout->addCell( progressBar1 );

		slider1 = colibriManager->createWidget<Colibri::Slider>( mainWindow );
		slider1->m_minSize = Ogre::Vector2( 350, 32 );
		layout->addCell( slider1 );

		sliderLabel = colibriManager->createWidget<Colibri::Label>( mainWindow );
		sliderLabel->sizeToFit();
		sliderLabel->m_minSize = Ogre::Vector2( 350, 32 );
		layout->addCell( sliderLabel );

		{
			const Colibri::LayoutCellVec &cells = layout->getCells();
			Colibri::LayoutCellVec::const_iterator itor = cells.begin();
			Colibri::LayoutCellVec::const_iterator end  = cells.end();

			while( itor != end )
			{
				(*itor)->m_margin = 5.0f;
				(*itor)->m_expand[0] = true;

				++itor;
			}
		}

		// Do not put slider2 in the layout
		slider2 = colibriManager->createWidget<Colibri::Slider>( vertWindow );
		slider2->m_minSize = Ogre::Vector2( 32, 320 );
		slider2->setSize( slider2->m_minSize );
		slider2->setVertical( true );

		layout->setAdjustableWindow( mainWindow );
		layout->m_hardMaxSize = colibriManager->getCanvasSize();

		Colibri::LayoutLine *layoutW = new Colibri::LayoutLine( colibriManager );
		layoutW->setCellSize( colibriManager->getCanvasSize() );
		layoutW->addCell( &Colibri::LayoutSpacer::c_DefaultBlankSpacer );
		layoutW->addCell( layout );
		layoutW->layout();


		//Overlapping windows
		demoActionListener = new DemoWidgetListener();

		Colibri::LayoutLine *layoutOverlapping = new Colibri::LayoutLine( colibriManager );
		overlapWindow1 = colibriManager->createWindow( 0 );
		overlapWindow1->setTransform(Ogre::Vector2(650, 250), Ogre::Vector2(300, 300));
		overlapWindow1->setZOrder(3);

		orderButtonBack = colibriManager->createWidget<Colibri::Button>( overlapWindow1 );
		orderButtonBack->m_minSize = Ogre::Vector2( 350, 64 );
		orderButtonBack->getLabel()->setText( "Send to back" );
		orderButtonBack->sizeToFit();

		orderButtonBack->addActionListener(demoActionListener);
		orderButtonFront = colibriManager->createWidget<Colibri::Button>( overlapWindow1 );
		orderButtonFront->m_minSize = Ogre::Vector2( 350, 64 );
		orderButtonFront->getLabel()->setText( "Bring to front" );
		orderButtonFront->sizeToFit();
		orderButtonFront->addActionListener(demoActionListener);

		overlapWindow2 = colibriManager->createWindow( 0 );
		overlapWindow2->setTransform(Ogre::Vector2(750, 450), Ogre::Vector2(300, 300));
		overlapWindow2->setZOrder(4);

		layoutOverlapping->addCell( orderButtonBack );
		layoutOverlapping->addCell( orderButtonFront );
		layoutOverlapping->layout();

		{
			innerOverlapWindow1 = colibriManager->createWindow( overlapWindow2 );
			innerOverlapWindow1->setTransform( Ogre::Vector2(10, 10), Ogre::Vector2(100, 100) );
			innerOverlapWindow1->setZOrder(1);
			innerOverlapWindow2 = colibriManager->createWindow( overlapWindow2 );
			innerOverlapWindow2->setTransform( Ogre::Vector2(20, 20), Ogre::Vector2(100, 100) );
			innerOverlapWindow2->setZOrder(4);

			innerOverlapButton1 = colibriManager->createWidget<Colibri::Button>( overlapWindow2 );
			innerOverlapButton1->getLabel()->setText( "First" );
			innerOverlapButton1->setTransform( Ogre::Vector2(150, 10), Ogre::Vector2(100, 100) );
			innerOverlapButton1->setZOrder(1);
			innerOverlapButton2 = colibriManager->createWidget<Colibri::Button>( overlapWindow2 );
			innerOverlapButton2->setTransform( Ogre::Vector2(160, 20), Ogre::Vector2(100, 100) );
			innerOverlapButton2->setZOrder(2);
			innerOverlapButton2->getLabel()->setText( "Second" );

			innerToggleOrder = colibriManager->createWidget<Colibri::Button>( overlapWindow2 );
			innerToggleOrder->m_minSize = Ogre::Vector2( 350, 64 );
			innerToggleOrder->getLabel()->setText( "Switch order" );
			innerToggleOrder->setTopLeft( Ogre::Vector2(0, 120) );
			innerToggleOrder->addActionListener(demoActionListener);
			innerToggleOrder->sizeToFit();
		}

#if 0
		//Colibri::LayoutLine *layout = new Colibri::LayoutLine( colibriManager );
		Colibri::LayoutMultiline *layout = new Colibri::LayoutMultiline( colibriManager );
		//Colibri::LayoutTableSameSize *layout = new Colibri::LayoutTableSameSize( colibriManager );

		layout->m_numLines = 2;
		//layout->m_numColumns = 3;
		//layout->m_transpose = false;
		//layout->m_softMaxSize = mainWindow->getSizeAfterClipping();
		//layout->m_evenMarginSpaceAtEdges = false;

		//layout->m_vertical = false;

		for( int i=0 ;i<4; ++i )
		{
			button1 = colibriManager->createWidget<Colibri::Button>( mainWindow );
			button1->setTopLeft( Ogre::Vector2( 192, 192 + 192 + 480 + i * 480 ) );
			//button1->setSize( i == 1 ? Ogre::Vector2( 288, 180 ) : Ogre::Vector2( 96, 54 ) );
			button1->setSize( Ogre::Vector2( 288, 180 ) );
			button1->getLabel()->setText( "Button " + Ogre::StringConverter::toString( i ) );

			//button1->setState( Colibri::States::Disabled );

			/*button1->m_proportion[0] = 1;
			button1->m_proportion[1] = 1;*/
			/*button1->m_expand[0] = true;
			button1->m_expand[1] = true;*/

			//layout->addCell( &Colibri::LayoutSpacer::c_DefaultBlankSpacer );
			//layout->addCell( new Colibri::LayoutSpacer() );
			button1->m_margin = Ogre::Vector2( 38 );
			//button1->m_gridLocation = Colibri::GridLocations::BottomRight;
			//button1->m_gridLocation = Colibri::GridLocations::TopRight;
			//button1->m_gridLocation = Colibri::GridLocations::Top;
			//button1->m_gridLocation = Colibri::GridLocations::TopLeft;
			//button1->m_gridLocation = Colibri::GridLocations::Center;
			layout->addCell( button1 );
		}

		//layout->addCell( &Colibri::LayoutSpacer::c_DefaultBlankSpacer );

		layout->layout();
#endif

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
		colibriManager->destroyWidget( orderButtonBack );
		colibriManager->destroyWidget( orderButtonFront );
		colibriManager->destroyWindow( mainWindow );
		colibriManager->destroyWindow( vertWindow );
		colibriManager->destroyWindow( overlapWindow1 );
		colibriManager->destroyWindow( overlapWindow2 );
		delete demoActionListener;
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
			//We tried to update this only inside ColibriGuiGameState::keyPressed
			//but it didn't work well in Linux with fcitx
			Ogre::Vector2 imeOffset = colibriManager->getImeLocation();
			SDL_Rect rect;
			rect.x = imeOffset.x;
			rect.y = imeOffset.y;
			rect.w = 0;
			rect.h = 0;
			if( oldRect.x != rect.x || oldRect.y != rect.y )
				SDL_SetTextInputRect( &rect );
		}

		/*static float angle = 0;
		Ogre::Matrix3 rotMat;
		rotMat.FromEulerAnglesXYZ( Ogre::Degree( 0 ), Ogre::Radian( 0 ), Ogre::Radian( angle ) );
		button->setOrientation( rotMat );
		angle += timeSinceLast;*/

		static float prevValue = -1.0f;
		float currentValue = slider1->getValue();
		if( prevValue != currentValue )
		{
			sliderLabel->setText( std::to_string(currentValue) );
			prevValue = currentValue;
		}

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
			Ogre::Vector2 mouseScroll( arg.wheel.x, -arg.wheel.y );
			colibriManager->setScroll( mouseScroll * 50.0f *
									   colibriManager->getCanvasSize() *
									   colibriManager->getPixelSize() );
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
	void ColibriGuiGameState::textEditing( const SDL_TextEditingEvent &arg )
	{
		colibriManager->setTextEdit( arg.text, arg.start, arg.length );
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::textInput( const SDL_TextInputEvent &arg )
	{
		colibriManager->setTextInput( arg.text );
	}
	//-----------------------------------------------------------------------------------
	void ColibriGuiGameState::keyPressed( const SDL_KeyboardEvent &arg )
	{
		const bool isTextInputActive = SDL_IsTextInputActive();
		const bool isTextMultiline = colibriManager->isTextMultiline();

		if( (arg.keysym.sym == SDLK_w && !isTextInputActive) || arg.keysym.sym == SDLK_UP )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Top );
		else if( (arg.keysym.sym == SDLK_s && !isTextInputActive) || arg.keysym.sym == SDLK_DOWN )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Bottom );
		else if( (arg.keysym.sym == SDLK_a && !isTextInputActive) || arg.keysym.sym == SDLK_LEFT )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Left );
		else if( (arg.keysym.sym == SDLK_d && !isTextInputActive) || arg.keysym.sym == SDLK_RIGHT )
			colibriManager->setKeyDirectionPressed( Colibri::Borders::Right );
		else if( ((arg.keysym.sym == SDLK_RETURN ||
				   arg.keysym.sym == SDLK_KP_ENTER) && !isTextMultiline) ||
				 (arg.keysym.sym == SDLK_SPACE && !isTextInputActive) )
		{
			colibriManager->setKeyboardPrimaryPressed();
		}
		else if( isTextInputActive )
		{
			if( (arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER) && isTextMultiline )
				colibriManager->setTextSpecialKeyPressed( SDLK_RETURN, arg.keysym.mod );
			else
			{
				colibriManager->setTextSpecialKeyPressed( arg.keysym.sym & ~SDLK_SCANCODE_MASK,
														  arg.keysym.mod );
			}
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

		const bool isTextInputActive = SDL_IsTextInputActive();
		const bool isTextMultiline = colibriManager->isTextMultiline();

		if( (arg.keysym.sym == SDLK_w && !isTextInputActive) || arg.keysym.sym == SDLK_UP )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Top );
		else if( (arg.keysym.sym == SDLK_s && !isTextInputActive) || arg.keysym.sym == SDLK_DOWN )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Bottom );
		else if( (arg.keysym.sym == SDLK_a && !isTextInputActive) || arg.keysym.sym == SDLK_LEFT )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Left );
		else if( (arg.keysym.sym == SDLK_d && !isTextInputActive) || arg.keysym.sym == SDLK_RIGHT )
			colibriManager->setKeyDirectionReleased( Colibri::Borders::Right );
		else if( ((arg.keysym.sym == SDLK_RETURN ||
				   arg.keysym.sym == SDLK_KP_ENTER) && !isTextMultiline) ||
				 (arg.keysym.sym == SDLK_SPACE && !isTextInputActive) )
		{
			colibriManager->setKeyboardPrimaryReleased();
		}
		else if( isTextInputActive )
		{
			if( (arg.keysym.sym == SDLK_RETURN || arg.keysym.sym == SDLK_KP_ENTER) && isTextMultiline )
				colibriManager->setTextSpecialKeyReleased( SDLK_RETURN, arg.keysym.mod );
			else
			{
				colibriManager->setTextSpecialKeyReleased( arg.keysym.sym & ~SDLK_SCANCODE_MASK,
														   arg.keysym.mod );
			}
		}

		TutorialGameState::keyReleased( arg );
	}
}
