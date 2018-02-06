
#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/CrystalManager.h"

namespace Crystal
{
	Window::Window( CrystalManager *manager ) :
		Renderable( manager ),
		m_defaultChildWidget( 0 ),
		m_widgetNavigationDirty( false ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false ),
		m_currentScroll( Ogre::Vector2::ZERO ),
		m_scrollArea( Ogre::Vector2::ZERO )
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
	UiVertex* Window::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
											  const Ogre::Vector2 &parentPos,
											  const Ogre::Matrix3 &parentRot )
	{
		if( !m_parent->intersects( this ) )
			return vertexBuffer;

		updateDerivedTransform( parentPos, parentRot );

		const Ogre::Vector2 topLeft		= this->m_derivedTopLeft;
		const Ogre::Vector2 bottomRight	= this->m_derivedBottomRight;

		vertexBuffer->x = topLeft.x;
		vertexBuffer->y = topLeft.y;
		vertexBuffer->clipDistance[Borders::Top]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Left]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Right]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Bottom]	= 1.0f;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		vertexBuffer->x = topLeft.x;
		vertexBuffer->y = bottomRight.y;
		vertexBuffer->clipDistance[Borders::Top]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Left]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Right]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Bottom]	= 1.0f;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		vertexBuffer->x = bottomRight.x;
		vertexBuffer->y = bottomRight.y;
		vertexBuffer->clipDistance[Borders::Top]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Left]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Right]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Bottom]	= 1.0f;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		vertexBuffer->x = bottomRight.x;
		vertexBuffer->y = topLeft.y;
		vertexBuffer->clipDistance[Borders::Top]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Left]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Right]	= 1.0f;
		vertexBuffer->clipDistance[Borders::Bottom]	= 1.0f;
		vertexBuffer->rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		vertexBuffer->rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		++vertexBuffer;

		const Ogre::Matrix3 finalRot = this->m_derivedOrientation;

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			vertexBuffer = (*itor)->fillBuffersAndCommands( vertexBuffer, topLeft, finalRot );
			++itor;
		}

		return vertexBuffer;
	}
}
