
#include "CrystalGui/CrystalWidget.h"
#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/CrystalManager.h"

namespace Crystal
{
	static Borders::Borders c_reciprocateBorders[Borders::NumBorders+1u] =
	{
		Borders::Bottom,
		Borders::Right,
		Borders::Left,
		Borders::Top,
		Borders::NumBorders
	};

	Widget::Widget( CrystalManager *manager ) :
		m_parent( 0 ),
		m_manager( manager ),
		m_hidden( false ),
		m_currentState( States::Idle ),
		m_position( Ogre::Vector2::ZERO ),
		m_size( Ogre::Vector2::ZERO ),
		m_orientation( Ogre::Matrix3::IDENTITY ),
		m_derivedTopLeft( Ogre::Vector2::ZERO ),
		m_derivedBottomRight( Ogre::Vector2::ZERO ),
		m_derivedOrientation( Ogre::Matrix3::IDENTITY )
	{
		memset( m_nextWidget, 0, sizeof(m_nextWidget) );
		for( size_t i=0; i<Borders::NumBorders; ++i )
			m_autoSetNextWidget[i] = true;
	}
	//-------------------------------------------------------------------------
	Widget::~Widget()
	{
		assert( m_children.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	void Widget::_destroy()
	{
		{
			WidgetVec::const_iterator itor = m_children.begin();
			WidgetVec::const_iterator end  = m_children.end();

			while( itor != end )
			{
				(*itor)->_destroy();
				delete *itor;
				++itor;
			}

			m_children.clear();
		}
		{
			WidgetListenerPairVec::const_iterator itor = m_listeners.begin();
			WidgetListenerPairVec::const_iterator end  = m_listeners.end();

			while( itor != end )
			{
				itor->listener->notifyWidgetDestroyed( this );
				++itor;
			}

			m_listeners.clear();
		}
	}
	//-------------------------------------------------------------------------
	void Widget::_setParent( Widget *parent )
	{
		assert( !this->m_parent && parent );
		this->m_parent = parent;
		parent->m_children.push_back( parent );
		parent->setWidgetNavigationDirty();
	}
	//-------------------------------------------------------------------------
	Window* Widget::getFirstParentWindow()
	{
		if( isWindow() )
		{
			assert( dynamic_cast<Window*>( this ) );
			return static_cast<Window*>( this );
		}

		return m_parent->getFirstParentWindow();
	}
	//-------------------------------------------------------------------------
	void Widget::notifyWidgetDestroyed( Widget *widget )
	{
		for( size_t i=0; i<Borders::NumBorders; ++i )
		{
			if( m_nextWidget[i] == widget )
				m_nextWidget[i] = 0;
		}

		WidgetVec::iterator itor = std::find( m_children.begin(), m_children.end(), widget );
		if( itor != m_children.end() )
			m_children.erase( itor );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::mul( const Ogre::Matrix3 &matrix, Ogre::Vector2 xyPos )
	{
		Ogre::Vector3 result = matrix * Ogre::Vector3( xyPos.x, xyPos.y, 0.0f );
		return Ogre::Vector2( result.x, result.y );
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransform( const Ogre::Vector2 &parentPos,
										 const Ogre::Matrix3 &parentRot )
	{
		const Ogre::Matrix3 finalRot = parentRot * m_orientation;

		m_derivedTopLeft		= mul( finalRot, m_position ) + parentPos;
		m_derivedBottomRight	= m_derivedTopLeft + mul( finalRot, m_size );
		m_derivedOrientation	= finalRot;
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransformFromParent()
	{
		updateDerivedTransform( m_parent->m_derivedTopLeft, m_parent->m_derivedOrientation );
	}
	//-------------------------------------------------------------------------
	WidgetListenerPairVec::iterator Widget::findListener( WidgetListener *listener )
	{
		WidgetListenerPairVec::iterator itor = m_listeners.begin();
		WidgetListenerPairVec::iterator end  = m_listeners.end();

		while( itor != end && itor->listener != listener )
			++itor;

		return itor;
	}
	//-------------------------------------------------------------------------
	void Widget::addListener( WidgetListener *listener )
	{
		WidgetListenerPairVec::iterator itor = findListener( listener );

		if( itor == m_listeners.end() )
		{
			WidgetListenerPair pair( listener );
			m_listeners.push_back( listener );
			itor = m_listeners.begin() + m_listeners.size() - 1u;
		}

		++itor->refCount;
	}
	//-------------------------------------------------------------------------
	void Widget::removeListener( WidgetListener *listener )
	{
		WidgetListenerPairVec::iterator itor = findListener( listener );

		if( itor != m_listeners.end() )
		{
			--itor->refCount;
			if( itor->refCount == 0 )
				Ogre::efficientVectorRemove( m_listeners, itor );
		}
		else
		{
			LogListener	*log = m_manager->getLogListener();
			log->log( "Widget::removeListener could not find the listener. Ignoring",
					  LogSeverity::Warning );
		}
	}
	//-------------------------------------------------------------------------
	void Widget::setWidgetNavigationDirty(void)
	{
		m_parent->setWidgetNavigationDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setNextWidget( Widget *nextWidget, Borders::Borders direction, bool reciprocate )
	{
		if( direction == Borders::NumBorders )
		{
			for( size_t i=0; i<Borders::NumBorders; ++i )
			{
				if( m_nextWidget[i] )
					m_nextWidget[i]->removeListener( this );
				m_nextWidget[i] = nextWidget;
				nextWidget->addListener( this );

				if( reciprocate )
					nextWidget->setNextWidget( this, c_reciprocateBorders[i], false );
			}
		}
		else
		{
			if( m_nextWidget[direction] )
				m_nextWidget[direction]->removeListener( this );
			m_nextWidget[direction] = nextWidget;
			nextWidget->addListener( this );

			if( reciprocate )
				nextWidget->setNextWidget( this, c_reciprocateBorders[direction], false );
		}
	}
	//-------------------------------------------------------------------------
	float Widget::getRight() const
	{
		return m_position.x + m_size.x;
	}
	//-------------------------------------------------------------------------
	float Widget::getBottom() const
	{
		return m_position.y + m_size.y;
	}
	//-------------------------------------------------------------------------
	bool Widget::intersects( Widget *widget ) const
	{
		return !( this->m_position.x > widget->m_position.x + widget->m_size.x	||
				  this->m_position.y > widget->m_position.y + widget->m_size.y	||
				  this->m_position.x + this->m_size.x < widget->m_position.x	||
				  this->m_position.y + this->m_size.y < widget->m_position.y );
	}
	//-------------------------------------------------------------------------
	UiVertex* Widget::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
											  const Ogre::Vector2 &parentPos,
											  const Ogre::Matrix3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );
		return vertexBuffer;
	}
	//-------------------------------------------------------------------------
	States::States Widget::getCurrentState() const
	{
		return m_currentState;
	}
	//-------------------------------------------------------------------------
	const WidgetVec& Widget::getChildren() const
	{
		return m_children;
	}
}
