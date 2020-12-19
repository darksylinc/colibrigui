
#include "ColibriGui/ColibriWindow.h"

#include "ColibriGui/ColibriManager.h"

#include "ColibriRenderable.inl"

#define TODO_should_flag_transforms_dirty

namespace Colibri
{
	Window::Window( ColibriManager *manager ) :
		Renderable( manager ),
		m_currentScroll( Ogre::Vector2::ZERO ),
		m_nextScroll( Ogre::Vector2::ZERO ),
		m_scrollableArea( Ogre::Vector2::ZERO ),
		m_defaultChildWidget( 0 ),
		m_widgetNavigationDirty( false ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false ),
		m_zOrderWindowDirty( false ),
		m_zOrderHasDirtyChildren( false ),
		m_zOrder( 0 )
	{
		m_childrenClickable = true;
	}
	//-------------------------------------------------------------------------
	Window::~Window()
	{
		COLIBRI_ASSERT( m_childWindows.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	Window* Window::getParentAsWindow() const
	{
		COLIBRI_ASSERT( dynamic_cast<Window*>( m_parent ) );
		Window *parentWindow = static_cast<Window*>( m_parent );
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
			//Remove ourselves from being our Window parent's child
			Window *parentWindow = getParentAsWindow();
			{
				WindowVec::iterator itor = std::find( parentWindow->m_childWindows.begin(),
													  parentWindow->m_childWindows.end(),
													  this );
				parentWindow->m_childWindows.erase( itor );
			}
			{
				WidgetVec::iterator itor = std::find( parentWindow->m_children.begin() +
													  parentWindow->getOffsetStartWindowChildren(),
													  parentWindow->m_children.end(),
													  this );
				parentWindow->m_children.erase( itor );
			}
		}

		{
			COLIBRI_ASSERT( m_childWindows.size() ==
							(m_children.size() - getOffsetStartWindowChildren()) );

			WindowVec childWindowsCopy = m_childWindows;
			WindowVec::const_iterator itor = childWindowsCopy.begin();
			WindowVec::const_iterator end  = childWindowsCopy.end();

			while( itor != end )
				m_manager->destroyWindow( *itor++ );

			m_childWindows.clear();
		}

		Renderable::_destroy();
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
	const Ogre::Vector2& Window::getScrollableArea() const
	{
		return m_scrollableArea;
	}
	//-------------------------------------------------------------------------
	bool Window::hasScroll() const
	{
		const Ogre::Vector2 maxScroll = getMaxScroll();
		const Ogre::Vector2 &pixelSize = m_manager->getPixelSize();
		return maxScroll.x >= pixelSize.x * 0.05f || maxScroll.y >= pixelSize.y * 0.05f;
	}
	//-------------------------------------------------------------------------
	void Window::sizeScrollToFit()
	{
		m_scrollableArea = calculateChildrenSize();
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Window::getCurrentScroll() const
	{
		return m_currentScroll;
	}
	//-------------------------------------------------------------------------
	bool Window::update( float timeSinceLast )
	{
		bool cursorFocusDirty = false;

		TODO_should_flag_transforms_dirty; //??? should we?
		const Ogre::Vector2 pixelSize = m_manager->getPixelSize();

		const Ogre::Vector2 maxScroll = getMaxScroll();

		if( m_nextScroll.y < 0.0f )
		{
			m_nextScroll.y = Ogre::Math::lerp( 0.0f, m_nextScroll.y,
											   exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.y > maxScroll.y )
		{
			m_nextScroll.y = Ogre::Math::lerp( maxScroll.y, m_nextScroll.y,
											   exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.x < 0.0f )
		{
			m_nextScroll.x = Ogre::Math::lerp( 0.0f, m_nextScroll.x,
											   exp2f( -15.0f * timeSinceLast ) );
		}
		if( m_nextScroll.x > maxScroll.x )
		{
			m_nextScroll.x = Ogre::Math::lerp( maxScroll.x, m_nextScroll.x,
											   exp2f( -15.0f * timeSinceLast ) );
		}

		if( fabs( m_currentScroll.x - m_nextScroll.x ) >= pixelSize.x ||
			fabs( m_currentScroll.y - m_nextScroll.y ) >= pixelSize.y )
		{
			m_currentScroll = Ogre::Math::lerp( m_nextScroll, m_currentScroll,
												exp2f( -15.0f * timeSinceLast ) );

			const Ogre::Vector2& mouseCursorPosNdc = m_manager->getMouseCursorPosNdc();
			if( this->intersects( mouseCursorPosNdc ) )
				cursorFocusDirty = true;
		}
		else
		{
			m_currentScroll = m_nextScroll;
		}

		WindowVec::const_iterator itor = m_childWindows.begin();
		WindowVec::const_iterator end  = m_childWindows.end();

		while( itor != end )
		{
			cursorFocusDirty |= (*itor)->update( timeSinceLast );
			++itor;
		}

		return cursorFocusDirty;
	}
	//-------------------------------------------------------------------------
	size_t Window::notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved )
	{
		const size_t idx = Widget::notifyParentChildIsDestroyed( childWidgetBeingRemoved );

		//If removing the child at index 0, keep the default as 0 rather than trying to subtract it.
		if( m_defaultChildWidget >= idx && m_defaultChildWidget != 0 )
			--m_defaultChildWidget;

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
	void Window::setZOrder( uint8_t z )
	{
		if( m_parent )
		{
			getParentAsWindow()->m_zOrderWindowDirty = true;
		}
		m_zOrder = z;
		notifyZOrderChildWindowIsDirty( true );
		//The above function sets this to true in the case of recursive calls up the tree.
		//However from here we know no children should be set as dirty, so set it back to false.
		m_zOrderHasDirtyChildren = false;
	}
	//-------------------------------------------------------------------------
	void Window::notifyZOrderChildWindowIsDirty( bool firstCall )
	{
		m_zOrderHasDirtyChildren = true;

		if( m_parent )
		{
			Window *parentWindow = getParentAsWindow();
			parentWindow->notifyZOrderChildWindowIsDirty( false );
		}
		else
		{
			m_manager->_setZOrderWindowDirty( firstCall );
		}
	}
	//-------------------------------------------------------------------------
	bool compareZOrder( const Window* w1, const Window* w2 )
	{
		return w1->getZOrder() < w2->getZOrder();
	}
	//-------------------------------------------------------------------------
	bool compareWidgetZOrder( const Widget* w1, const Widget* w2 )
	{
		if( !w1->isWindow() || !w2->isWindow() ) return false;
		return dynamic_cast<const Window*>(w1)->getZOrder() < dynamic_cast<const Window*>(w2)->getZOrder();
	}
	//-------------------------------------------------------------------------
	void Window::reorderWindowVec( bool windowInListDirty, WindowVec& win, WidgetVec* widgetVec )
	{
		if( windowInListDirty )
		{
			std::stable_sort( win.begin(), win.end(), compareZOrder );
			if( widgetVec )
			{
				std::stable_sort( widgetVec->begin(), widgetVec->end(), compareWidgetZOrder );
			}
		}

		WindowVec::iterator itor = win.begin();
		WindowVec::iterator end  = win.end();

		while( itor != end )
		{
			if( (*itor)->getZOrderHasDirtyChildren() )
			{
				(*itor)->updateZOrderDirty();
			}
			++itor;
		}

	}
	//-------------------------------------------------------------------------
	void Window::updateZOrderDirty()
	{
		Window::reorderWindowVec( getZOrderWindowDirty(), m_childWindows, &m_children );
		m_zOrderWindowDirty = false;
		m_zOrderHasDirtyChildren = false;
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
			LogListener	*log = m_manager->getLogListener();
			log->log( "Window::removeChild could not find the window. It's not our child.",
					  LogSeverity::Fatal );
		}
		else
		{
			m_childWindows.erase( itor );
			window->m_parent = 0;

			WidgetVec::iterator itWidget = std::find( m_children.begin() + m_numWidgets,
													  m_children.end(), window );
			if( itWidget == m_children.end() )
			{
				LogListener	*log = m_manager->getLogListener();
				log->log( "Window::removeChild could not find the window in "
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
		WidgetVec::const_iterator itor = std::find( m_children.begin(),
													m_children.end(), widget );
		m_defaultChildWidget = itor - m_children.begin();
		COLIBRI_ASSERT( m_defaultChildWidget < m_numWidgets );
	}
	//-------------------------------------------------------------------------
	Widget* colibrigui_nullable Window::getDefaultWidget() const
	{
		Widget *retVal = 0;

		size_t numChildren = m_numWidgets;

		size_t defaultChild = m_defaultChildWidget;
		for( size_t i=0; i<numChildren && !retVal; ++i )
		{
			Widget *child = m_children[defaultChild];
			if( child->getCurrentState() != States::Disabled )
				retVal = child;

			defaultChild = (defaultChild + 1u) % numChildren;
		}

		if( !retVal && m_numWidgets > 0 )
			retVal = m_children.front();

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Window::_updateDerivedTransformOnly( const Ogre::Vector2 &parentPos,
											  const Matrix2x3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );

		const Ogre::Vector2 invCanvasSize2x	= m_manager->getInvCanvasSize2x();
		const Ogre::Vector2 outerTopLeft	= this->m_derivedTopLeft;
		const Ogre::Vector2 outerTopLeftWithClipping = outerTopLeft +
													   (m_clipBorderTL - m_currentScroll) *
													   invCanvasSize2x;

		const Matrix2x3 &finalRot = this->m_derivedOrientation;
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->_updateDerivedTransformOnly( outerTopLeftWithClipping, finalRot );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Window::_fillBuffersAndCommands( UiVertex ** RESTRICT_ALIAS vertexBuffer,
										 GlyphVertex ** RESTRICT_ALIAS textVertBuffer,
										 const Ogre::Vector2 &parentPos,
										 const Ogre::Vector2 &parentCurrentScrollPos,
										 const Matrix2x3 &parentRot )
	{
		Renderable::_fillBuffersAndCommands( vertexBuffer, textVertBuffer, parentPos,
											 parentCurrentScrollPos, parentRot, m_currentScroll, true );
	}
}
