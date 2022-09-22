
#include "ColibriGui/ColibriWindow.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"

#include "ColibriRenderable.inl"

#define TODO_should_flag_transforms_dirty

namespace Colibri
{
	Window::Window( ColibriManager *manager ) :
		Renderable( manager ),
		m_currentScroll( Ogre::Vector2::ZERO ),
		m_nextScroll( Ogre::Vector2::ZERO ),
		m_scrollableArea( Ogre::Vector2::ZERO ),
		m_defaultChildWidget( 0u ),
		m_lastPrimaryAction( std::numeric_limits<uint16_t>::max() ),
		m_widgetNavigationDirty( false ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false )
	{
		memset( m_arrows, 0, sizeof( m_arrows ) );
		memset( m_scrollArrowsVisibility, 0, sizeof( m_scrollArrowsVisibility ) );
		memset( m_scrollArrowProportion, 0, sizeof( m_scrollArrowProportion ) );

		for( size_t i = 0u; i < Borders::NumBorders; ++i )
		{
			// If there is a skin defined for the arrows, default to show them.
			// Later evaluateScrollArrowVisibility will create the arrow if needed.
			const SkinWidgetTypes::SkinWidgetTypes defaultSkinType =
				static_cast<SkinWidgetTypes::SkinWidgetTypes>( SkinWidgetTypes::WindowArrowScrollTop +
															   i );
			if( m_manager->getDefaultSkin( defaultSkinType )[0] )
				m_scrollArrowsVisibility[i] = true;
		}

		m_childrenClickable = true;
		m_zOrder = _wrapZOrderInternalId( 0 );
		setConsumeCursor( true );
	}
	//-------------------------------------------------------------------------
	Window::~Window()
	{
		COLIBRI_ASSERT( m_childWindows.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	Window *Window::getParentAsWindow() const
	{
		COLIBRI_ASSERT( dynamic_cast<Window *>( m_parent ) );
		Window *parentWindow = static_cast<Window *>( m_parent );
		return parentWindow;
	}
	//-------------------------------------------------------------------------
	void Window::_initialize()
	{
		_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::Window ) );
		Renderable::_initialize();
	}
	//-------------------------------------------------------------------------
	void Window::_destroy()
	{
		m_destructionStarted = true;
		setWindowNavigationDirty();

		if( m_parent )
		{
			// Remove ourselves from being our Window parent's child
			Window *parentWindow = getParentAsWindow();
			{
				WindowVec::iterator itor = std::find( parentWindow->m_childWindows.begin(),
													  parentWindow->m_childWindows.end(), this );
				parentWindow->m_childWindows.erase( itor );
			}
			{
				WidgetVec::iterator itor = std::find(
					parentWindow->m_children.begin() + parentWindow->getOffsetStartWindowChildren(),
					parentWindow->m_children.end(), this );
				parentWindow->m_children.erase( itor );
			}
		}

		{
			COLIBRI_ASSERT( m_childWindows.size() ==
							( m_children.size() - getOffsetStartWindowChildren() ) );

			WindowVec childWindowsCopy = m_childWindows;
			WindowVec::const_iterator itor = childWindowsCopy.begin();
			WindowVec::const_iterator end = childWindowsCopy.end();

			while( itor != end )
				m_manager->destroyWindow( *itor++ );

			m_childWindows.clear();
		}

		Renderable::_destroy();
	}
	//-------------------------------------------------------------------------
	inline bool Window::isArrowBreadthFirstReady( const Widget *arrow ) const
	{
		return arrow->getParent() != this;
	}
	//-------------------------------------------------------------------------
	void Window::evaluateScrollArrowVisibility( Borders::Borders border )
	{
		const Ogre::Vector2 maxScroll = getMaxScroll();

		if( ( ( border == Borders::Left || border == Borders::Right ) &&
			  std::abs( maxScroll.x ) <= 1e-6f ) ||
			( ( border == Borders::Top || border == Borders::Bottom ) &&
			  std::abs( maxScroll.y ) <= 1e-6f ) )
		{
			// Window is not scrollable. Destroy the arrow
			if( m_arrows[border] )
			{
				m_manager->destroyWidget( m_arrows[border] );
				m_arrows[border] = 0;
			}

			return;
		}

		// Window IS scrollable
		if( !m_scrollArrowsVisibility[border] )
		{
			// Explicitly requested not to show the arrows.
			if( m_arrows[border] )
				m_arrows[border]->setHidden( true );

			return;
		}

		// If we reach here, the arrow must exist
		if( !m_arrows[border] )
		{
			createScrollArrow( border );
		}
		else if( m_breadthFirst != isArrowBreadthFirstReady( m_arrows[border] ) )
		{
			// m_breadthFirst changed, we must recreate the arrow
			m_manager->destroyWidget( m_arrows[border] );
			m_arrows[border] = 0;
			createScrollArrow( border );
		}

		m_arrows[border]->setCenter( m_size * m_scrollArrowProportion[border] );
		const Ogre::Vector2 arrowTopLeft = m_arrows[border]->getLocalTopLeft();

		// Whether the arrow is shown depends on current scroll.
		// We also need to keep the position updated.
		switch( border )
		{
		case Borders::Top:
			m_arrows[border]->setHidden( m_currentScroll.y <= 0.0f );
			m_arrows[border]->setTopLeft( Ogre::Vector2( arrowTopLeft.x, 0.0f ) + m_currentScroll );
			break;
		case Borders::Left:
			m_arrows[border]->setHidden( m_currentScroll.x <= 0.0f );
			m_arrows[border]->setTopLeft( Ogre::Vector2( 0.0f, arrowTopLeft.y ) + m_currentScroll );
			break;
		case Borders::Right:
			m_arrows[border]->setHidden( m_currentScroll.x >= maxScroll.x );
			m_arrows[border]->setTopLeft(
				Ogre::Vector2( getSizeAfterClipping().x - m_arrows[border]->getSize().x,
							   arrowTopLeft.y ) +
				m_currentScroll );
			break;
		case Borders::Bottom:
			m_arrows[border]->setHidden( m_currentScroll.y >= maxScroll.y );
			m_arrows[border]->setTopLeft(
				Ogre::Vector2( arrowTopLeft.x,
							   getSizeAfterClipping().y - m_arrows[border]->getSize().y ) +
				m_currentScroll );
			break;
		case Borders::NumBorders:
			break;
		}
	}
	//-------------------------------------------------------------------------
	void Window::createScrollArrow( Borders::Borders border )
	{
		COLIBRI_ASSERT_LOW( !m_arrows[border] );

		const SkinWidgetTypes::SkinWidgetTypes defaultSkinType =
			static_cast<SkinWidgetTypes::SkinWidgetTypes>( SkinWidgetTypes::WindowArrowScrollTop +
														   border );
		const SkinManager *skinManager = m_manager->getSkinManager();
		const Ogre::IdString skinPackName = m_manager->getDefaultSkinPackName( defaultSkinType );
		const SkinPack *defaultSkinPack = skinManager->findSkinPack( skinPackName, LogSeverity::Fatal );

		m_scrollArrowProportion[border] = defaultSkinPack->windowScrollArrowProportion;

		Widget *dummyFirstWidget = 0;
		if( m_breadthFirst )
		{
			// HACK: In breadth first, the contents of the window will often end up partially
			// on top of our arrows, since by design overlapping causes problems in this mode.
			// To workaround this, we create a widget that is a child of a widget that contains
			// the arrow.
			dummyFirstWidget = m_manager->createWidget<Widget>( this );
			dummyFirstWidget->setZOrder( 255u );
			dummyFirstWidget->setPressable( false );
			dummyFirstWidget->setSize( Ogre::Vector2( defaultSkinPack->windowScrollArrowSize ) );

			m_arrows[border] = dummyFirstWidget;
			for( int i = 0; i < 2; ++i )
			{
				m_arrows[border] = m_manager->createWidget<Widget>( m_arrows[border] );
				m_arrows[border]->setSize( Ogre::Vector2( defaultSkinPack->windowScrollArrowSize ) );
			}
			m_arrows[border] = m_manager->createWidget<Renderable>( m_arrows[border] );
		}
		else
		{
			m_arrows[border] = m_manager->createWidget<Renderable>( this );
		}

		COLIBRI_ASSERT_HIGH( dynamic_cast<Renderable *>( m_arrows[border] ) );
		static_cast<Renderable *>( m_arrows[border] )
			->_setSkinPack( m_manager->getDefaultSkin( defaultSkinType ) );
		m_arrows[border]->setSize( Ogre::Vector2( defaultSkinPack->windowScrollArrowSize ) );
		m_arrows[border]->setZOrder( 255u );
		m_arrows[border] = dummyFirstWidget;
	}
	//-------------------------------------------------------------------------
	void Window::setScrollVisible( bool bVisible, Borders::Borders border )
	{
		if( border == Borders::NumBorders )
		{
			for( size_t i = 0u; i < Borders::NumBorders; ++i )
				setScrollVisible( bVisible, static_cast<Borders::Borders>( i ) );
		}
		else
		{
			m_scrollArrowsVisibility[border] = bVisible;
			evaluateScrollArrowVisibility( border );
		}
	}
	//-------------------------------------------------------------------------
	bool Window::getScrollVisible( Borders::Borders border ) const
	{
		COLIBRI_ASSERT_LOW( border != Borders::NumBorders );
		if( border == Borders::NumBorders )
			return false;
		return m_scrollArrowsVisibility[border];
	}
	//-------------------------------------------------------------------------
	void Window::setScrollAnimated( const Ogre::Vector2 &nextScroll, bool animateOutOfRange )
	{
		m_nextScroll = nextScroll;
		if( !animateOutOfRange )
		{
			const Ogre::Vector2 maxScroll = getMaxScroll();
			m_nextScroll.makeFloor( maxScroll );
			m_nextScroll.makeCeil( Ogre::Vector2::ZERO );
		}
		else
		{
			if( !hasScrollX() )
				m_nextScroll.x = 0.0f;
			if( !hasScrollY() )
				m_nextScroll.y = 0.0f;
		}
	}
	//-------------------------------------------------------------------------
	void Window::setScrollImmediate( const Ogre::Vector2 &scroll )
	{
		m_currentScroll = scroll;
		const Ogre::Vector2 maxScroll = getMaxScroll();
		m_currentScroll.makeFloor( maxScroll );
		m_currentScroll.makeCeil( Ogre::Vector2::ZERO );
		m_nextScroll = m_currentScroll;
	}
	//-------------------------------------------------------------------------
	void Window::setMaxScroll( const Ogre::Vector2 &maxScroll )
	{
		COLIBRI_ASSERT_LOW( maxScroll.x >= 0 && maxScroll.y >= 0 );
		m_scrollableArea = maxScroll - m_clipBorderBR - m_clipBorderTL + m_size;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Window::getMaxScroll() const
	{
		Ogre::Vector2 maxScroll = m_scrollableArea - m_size + m_clipBorderTL + m_clipBorderBR;
		maxScroll.makeCeil( Ogre::Vector2::ZERO );
		return maxScroll;
	}
	//-------------------------------------------------------------------------
	void Window::setScrollableArea( const Ogre::Vector2 &scrollableArea )
	{
		COLIBRI_ASSERT_LOW( m_scrollableArea.x >= 0 && m_scrollableArea.y >= 0 );
		m_scrollableArea = scrollableArea;
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2 &Window::getScrollableArea() const { return m_scrollableArea; }
	//-------------------------------------------------------------------------
	bool Window::hasScroll() const
	{
		const Ogre::Vector2 maxScroll = getMaxScroll();
		const Ogre::Vector2 &pixelSize = m_manager->getPixelSize();
		return maxScroll.x >= pixelSize.x * 0.05f || maxScroll.y >= pixelSize.y * 0.05f;
	}
	//-------------------------------------------------------------------------
	bool Window::hasScrollX() const
	{
		const Ogre::Vector2 maxScroll = getMaxScroll();
		const Ogre::Vector2 &pixelSize = m_manager->getPixelSize();
		return maxScroll.x >= pixelSize.x * 0.05f;
	}
	//-------------------------------------------------------------------------
	bool Window::hasScrollY() const
	{
		const Ogre::Vector2 maxScroll = getMaxScroll();
		const Ogre::Vector2 &pixelSize = m_manager->getPixelSize();
		return maxScroll.y >= pixelSize.y * 0.05f;
	}
	//-------------------------------------------------------------------------
	void Window::sizeScrollToFit() { m_scrollableArea = calculateChildrenSize(); }
	//-------------------------------------------------------------------------
	const Ogre::Vector2 &Window::getCurrentScroll() const { return m_currentScroll; }
	//-------------------------------------------------------------------------
	bool Window::update( float timeSinceLast )
	{
		bool cursorFocusDirty = false;

		TODO_should_flag_transforms_dirty;  //??? should we?
		const Ogre::Vector2 pixelSize = m_manager->getPixelSize();

		const Ogre::Vector2 maxScroll = getMaxScroll();

		if( m_nextScroll.y < 0.0f )
		{
			m_nextScroll.y = Ogre::Math::lerp( 0.0f, m_nextScroll.y, exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.y > maxScroll.y )
		{
			m_nextScroll.y =
				Ogre::Math::lerp( maxScroll.y, m_nextScroll.y, exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.x < 0.0f )
		{
			m_nextScroll.x = Ogre::Math::lerp( 0.0f, m_nextScroll.x, exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.x > maxScroll.x )
		{
			m_nextScroll.x =
				Ogre::Math::lerp( maxScroll.x, m_nextScroll.x, exp2f( -15.0f * timeSinceLast ) );
		}

		if( fabs( m_currentScroll.x - m_nextScroll.x ) >= pixelSize.x ||
			fabs( m_currentScroll.y - m_nextScroll.y ) >= pixelSize.y )
		{
			m_currentScroll =
				Ogre::Math::lerp( m_nextScroll, m_currentScroll, exp2f( -15.0f * timeSinceLast ) );

			const Ogre::Vector2 &mouseCursorPosNdc = m_manager->getMouseCursorPosNdc();
			if( this->intersects( mouseCursorPosNdc ) )
				cursorFocusDirty = true;
		}
		else
		{
			m_currentScroll = m_nextScroll;
		}

		for( size_t i = 0u; i < Borders::NumBorders; ++i )
			evaluateScrollArrowVisibility( static_cast<Borders::Borders>( i ) );

		WindowVec::const_iterator itor = m_childWindows.begin();
		WindowVec::const_iterator endt = m_childWindows.end();

		while( itor != endt )
		{
			cursorFocusDirty |= ( *itor )->update( timeSinceLast );
			++itor;
		}

		return cursorFocusDirty;
	}
	//-------------------------------------------------------------------------
	size_t Window::notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved )
	{
		const size_t idx = Widget::notifyParentChildIsDestroyed( childWidgetBeingRemoved );

		// If removing the child at index 0, keep the default as 0 rather than trying to subtract it.
		if( m_defaultChildWidget >= idx && m_defaultChildWidget != 0 )
			--m_defaultChildWidget;

		if( m_lastPrimaryAction > idx && m_lastPrimaryAction != 0u &&
			m_lastPrimaryAction != std::numeric_limits<uint16_t>::max() )
		{
			--m_lastPrimaryAction;
		}
		else if( m_lastPrimaryAction == idx )
		{
			// Last known button is being removed. Use the default widget instead
			m_lastPrimaryAction = std::numeric_limits<uint16_t>::max();
		}

		return idx;
	}
	//-------------------------------------------------------------------------
	void Window::notifyChildWindowIsDirty()
	{
		m_childrenNavigationDirty = true;

		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->notifyChildWindowIsDirty();
		}
		else
		{
			m_manager->_setWindowNavigationDirty();
		}
	}
	//-------------------------------------------------------------------------
	void Window::reorderWidgetVec( bool widgetInListDirty, WidgetVec &widgets )
	{
		Widget::reorderWidgetVec( widgetInListDirty, widgets );
		// Sort the windows as well as the widgets to keep them in the same order.
		if( widgetInListDirty )
		{
			std::stable_sort( m_childWindows.begin(), m_childWindows.end(), _compareWidgetZOrder );
		}
	}
	//-------------------------------------------------------------------------
	void Window::setWidgetNavigationDirty()
	{
		m_widgetNavigationDirty = true;

		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->notifyChildWindowIsDirty();
		}
	}
	//-------------------------------------------------------------------------
	void Window::setWindowNavigationDirty()
	{
		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->m_windowNavigationDirty = true;
			parentWindow->notifyChildWindowIsDirty();
		}
		else
		{
			m_manager->_setWindowNavigationDirty();
		}
	}
	//-------------------------------------------------------------------------
	void Window::attachChild( Window *window )
	{
		window->detachFromParent();
		m_childWindows.push_back( window );
		window->_setParent( this );
	}
	//-------------------------------------------------------------------------
	void Window::detachChild( Window *window )
	{
		WindowVec::iterator itor = std::find( m_childWindows.begin(), m_childWindows.end(), window );

		if( itor == m_childWindows.end() )
		{
			LogListener *log = m_manager->getLogListener();
			log->log( "Window::removeChild could not find the window. It's not our child.",
					  LogSeverity::Fatal );
		}
		else
		{
			m_childWindows.erase( itor );
			window->m_parent = 0;

			WidgetVec::iterator itWidget =
				std::find( m_children.begin() + m_numWidgets, m_children.end(), window );
			if( itWidget == m_children.end() )
			{
				LogListener *log = m_manager->getLogListener();
				log->log(
					"Window::removeChild could not find the window in "
					"m_children but it was in m_childWindows!",
					LogSeverity::Fatal );
			}
			else
			{
				m_children.erase( itWidget );
			}

			m_manager->_setAsParentlessWindow( window );
		}
	}
	//-------------------------------------------------------------------------
	void Window::detachFromParent()
	{
		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->detachChild( this );
		}
	}
	//-------------------------------------------------------------------------
	void Window::setDefault( Widget *widget )
	{
		COLIBRI_ASSERT( !widget->isWindow() );
		WidgetVec::const_iterator itor = std::find( m_children.begin(), m_children.end(), widget );
		m_defaultChildWidget = itor - m_children.begin();
		COLIBRI_ASSERT( m_defaultChildWidget < m_numWidgets );
	}
	//-------------------------------------------------------------------------
	Widget *colibrigui_nullable Window::getDefaultWidget() const
	{
		Widget *retVal = 0;

		size_t numChildren = m_numWidgets;

		size_t defaultChild = m_defaultChildWidget;
		for( size_t i = 0; i < numChildren && !retVal; ++i )
		{
			Widget *child = m_children[defaultChild];
			if( child->isKeyboardNavigable() )
				retVal = child;

			defaultChild = ( defaultChild + 1u ) % numChildren;
		}

		if( !retVal && m_numWidgets > 0 )
			retVal = m_children.front();

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Window::setLastPrimaryAction( Widget *widget )
	{
		COLIBRI_ASSERT( !widget->isWindow() );
		WidgetVec::const_iterator itor = std::find( m_children.begin(), m_children.end(), widget );
		m_lastPrimaryAction = static_cast<uint16_t>( itor - m_children.begin() );
		COLIBRI_ASSERT( m_lastPrimaryAction < m_numWidgets );
	}
	//-------------------------------------------------------------------------
	Widget *colibrigui_nullable Window::getLastPrimaryAction() const
	{
		Widget *retVal = 0;

		if( m_lastPrimaryAction > m_numWidgets || m_children[m_lastPrimaryAction]->isDisabled() )
			retVal = getDefaultWidget();
		else
			retVal = m_children[m_lastPrimaryAction];

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Window::_updateDerivedTransformOnly( const Ogre::Vector2 &parentPos,
											  const Matrix2x3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );

		const Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();
		const Ogre::Vector2 outerTopLeft = this->m_derivedTopLeft;
		const Ogre::Vector2 outerTopLeftWithClipping =
			outerTopLeft + ( m_clipBorderTL - m_currentScroll ) * invCanvasSize2x;

		const Matrix2x3 &finalRot = this->m_derivedOrientation;
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end = m_children.end();

		while( itor != end )
		{
			( *itor )->_updateDerivedTransformOnly( outerTopLeftWithClipping, finalRot );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Window::_fillBuffersAndCommands( UiVertex **RESTRICT_ALIAS vertexBuffer,
										  GlyphVertex **RESTRICT_ALIAS textVertBuffer,
										  const Ogre::Vector2 &parentPos,
										  const Ogre::Vector2 &parentCurrentScrollPos,
										  const Matrix2x3 &parentRot )
	{
		Renderable::_fillBuffersAndCommands( vertexBuffer, textVertBuffer, parentPos,
											 parentCurrentScrollPos, parentRot, m_currentScroll, true );
	}
}  // namespace Colibri
