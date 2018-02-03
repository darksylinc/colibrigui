
#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/CrystalManager.h"

namespace Crystal
{
	Window::Window( CrystalManager *manager ) :
		Renderable( manager ),
		m_defaultChildWidget( 0 ),
		m_widgetNavigationDirty( false ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false )
	{
	}
	//-------------------------------------------------------------------------
	Window::~Window()
	{
		assert( m_childWindows.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	Window* Window::getParentAsWindow() const
	{
		assert( dynamic_cast<Window*>( m_parent ) );
		Window *parentWindow = static_cast<Window*>( m_parent );
		return parentWindow;
	}
	//-------------------------------------------------------------------------
	void Window::_destroy()
	{
		{
			WindowVec::const_iterator itor = m_childWindows.begin();
			WindowVec::const_iterator end  = m_childWindows.end();

			while( itor != end )
			{
				(*itor)->_destroy();
				delete *itor;
				++itor;
			}

			m_childWindows.clear();
		}

		Widget::_destroy();
	}
	//-------------------------------------------------------------------------
	void Window::notifyWidgetDestroyed( Widget *widget )
	{
		WidgetVec::iterator itor = std::find( m_children.begin(), m_children.end(), widget );
		if( itor != m_children.end() )
		{
			const size_t idx = itor - m_children.begin();
			if( m_defaultChildWidget >= idx )
				--m_defaultChildWidget;

			m_children.erase( itor );
		}

		Widget::notifyWidgetDestroyed( widget );
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
			m_manager->_notifyChildWindowIsDirty();
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
	}
	//-------------------------------------------------------------------------
	void Window::detachChild( Window *window )
	{
		WindowVec::iterator itor = std::find( m_childWindows.begin(), m_childWindows.end(), window );

		if( itor != m_childWindows.end() )
		{
			LogListener	*log = m_manager->getLogListener();
			log->log( "Window::removeChild could not find the window. It's not our child.",
					  LogSeverity::Fatal );
		}
		else
		{
			m_childWindows.erase( itor );
			window->m_parent = 0;
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
		WidgetVec::const_iterator itor = std::find( m_children.begin(),
													m_children.end(), widget );
		m_defaultChildWidget = itor - m_children.begin();
	}
	//-------------------------------------------------------------------------
	Widget* crystalgui_nullable Window::getDefaultWidget() const
	{
		Widget *retVal = 0;

		size_t numChildren = m_children.size();

		size_t defaultChild = m_defaultChildWidget;
		for( size_t i=0; i<numChildren && !retVal; ++i )
		{
			Widget *child = m_children[defaultChild];
			if( child->getCurrentState() != States::Disabled )
				retVal = child;

			defaultChild = (defaultChild + 1u) % numChildren;
		}

		if( !retVal && !m_children.empty() )
			retVal = m_children.front();

		return retVal;
	}
}
