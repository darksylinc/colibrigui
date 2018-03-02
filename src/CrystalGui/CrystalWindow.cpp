
#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/CrystalManager.h"

#include "CrystalRenderable.inl"

namespace Crystal
{
	Window::Window( CrystalManager *manager ) :
		Renderable( manager ),
		m_currentScroll( Ogre::Vector2::ZERO ),
		m_scrollArea( Ogre::Vector2::ZERO ),
		m_allowsFocusWithChildren( true ),
		m_defaultChildWidget( 0 ),
		m_widgetNavigationDirty( false ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false )
	{
	}
	//-------------------------------------------------------------------------
	Window::~Window()
	{
		CRYSTAL_ASSERT( m_childWindows.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	Window* Window::getParentAsWindow() const
	{
		CRYSTAL_ASSERT( dynamic_cast<Window*>( m_parent ) );
		Window *parentWindow = static_cast<Window*>( m_parent );
		return parentWindow;
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
			WindowVec::iterator itor = std::find( parentWindow->m_childWindows.begin(),
												  parentWindow->m_childWindows.end(),
												  this );
			parentWindow->m_childWindows.erase( itor );
		}

		{
			WindowVec::const_iterator itor = m_childWindows.begin();
			WindowVec::const_iterator end  = m_childWindows.end();

			while( itor != end )
				m_manager->destroyWindow( *itor++ );

			m_childWindows.clear();
		}

		Renderable::_destroy();
	}
	//-------------------------------------------------------------------------
	size_t Window::notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved )
	{
		const size_t idx = Widget::notifyParentChildIsDestroyed( childWidgetBeingRemoved );

		if( m_defaultChildWidget >= idx )
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
		CRYSTAL_ASSERT( !widget->isWindow() );
		WidgetVec::const_iterator itor = std::find( m_children.begin(),
													m_children.end(), widget );
		m_defaultChildWidget = itor - m_children.begin();
		CRYSTAL_ASSERT( m_defaultChildWidget < m_numWidgets );
	}
	//-------------------------------------------------------------------------
	Widget* crystalgui_nullable Window::getDefaultWidget() const
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
	UiVertex* Window::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
											  const Ogre::Vector2 &parentPos,
											  const Ogre::Matrix3 &parentRot )
	{
		return Renderable::fillBuffersAndCommands( vertexBuffer, parentPos, parentRot, true );
	}
	//-------------------------------------------------------------------------
	FocusPair Window::setIdleCursorMoved( const Ogre::Vector2 &newPosInCanvas )
	{
		FocusPair retVal;

		//The first window that our button is touching wins. We go in LIFO order.
		WindowVec::const_reverse_iterator ritor = m_childWindows.rbegin();
		WindowVec::const_reverse_iterator rend  = m_childWindows.rend();

		while( ritor != rend && !retVal.widget )
		{
			retVal = (*ritor)->setIdleCursorMoved( newPosInCanvas );
			++ritor;
		}

		//One of the child windows is being touched by the cursor. We're done.
		if( retVal.widget )
			return retVal;

		if( !this->intersects( newPosInCanvas ) || !m_allowsFocusWithChildren )
			return FocusPair();

		WidgetVec::const_iterator itor = m_children.begin() + m_numNonRenderables;
		WidgetVec::const_iterator end  = m_children.begin() + m_numWidgets;

		while( itor != end && !retVal.widget )
		{
			Widget *widget = *itor;
			if( this->intersectsChild( widget ) &&
				widget->intersects( newPosInCanvas ) )
			{

				retVal.widget = widget;
			}
			++itor;
		}

		retVal.window = this;

		return retVal;
	}
}
