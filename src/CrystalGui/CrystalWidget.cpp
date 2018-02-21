
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
		m_size( manager->getCanvasSize() ),
		m_orientation( Ogre::Matrix3::IDENTITY ),
		m_derivedTopLeft( Ogre::Vector2::ZERO ),
		m_derivedBottomRight( Ogre::Vector2::ZERO ),
		m_derivedOrientation( Ogre::Matrix3::IDENTITY )
  #if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_LOW
	,	m_transformOutOfDate( false )
  #endif
	{
		memset( m_nextWidget, 0, sizeof(m_nextWidget) );
		for( size_t i=0; i<Borders::NumBorders; ++i )
			m_autoSetNextWidget[i] = true;
	}
	//-------------------------------------------------------------------------
	Widget::~Widget()
	{
		CRYSTAL_ASSERT( m_children.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	size_t Widget::notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved )
	{
		//Remove ourselves from being our parent's child
		WidgetVec::iterator itor = std::find( m_children.begin(), m_children.end(),
											  childWidgetBeingRemoved );
		const size_t idx = itor - m_children.begin();
		m_children.erase( itor );
		return idx;
	}
	//-------------------------------------------------------------------------
	void Widget::_destroy()
	{
		setWidgetNavigationDirty();

		for( size_t i=0; i<Borders::NumBorders; ++i )
		{
			//Tell other widgets to not notify us anymore. Otherwise they'll call a dangling pointer
			if( m_nextWidget[i] )
			{
				m_nextWidget[i]->removeListener( this );
				m_nextWidget[i] = 0;
			}
		}

		if( !isWindow() )
		{
			//Remove ourselves from being our parent's child
			m_parent->notifyParentChildIsDestroyed( this );
		}

		{
			//Move m_children to a temporary container; otherwise these children
			//will try to alter m_children while we're iterating (to remove themselves
			//from us) thus corrupting the iterators and wasting lots of cycles in
			//linear searches.
			WidgetVec children;
			children.swap( m_children );
			WidgetVec::const_iterator itor = children.begin();
			WidgetVec::const_iterator end  = children.end();

			while( itor != end )
				m_manager->destroyWidget( *itor++ );

			children.swap( m_children );
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
		CRYSTAL_ASSERT( !this->m_parent && parent );
		CRYSTAL_ASSERT( !this->isWindow() );
		this->m_parent = parent;
		parent->m_children.push_back( parent );
		parent->setWidgetNavigationDirty();
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	Window* Widget::getFirstParentWindow()
	{
		if( isWindow() )
		{
			CRYSTAL_ASSERT( dynamic_cast<Window*>( this ) );
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

		/* Do not remove it from m_children. It's unnecessary because the relationship
		   is implicit (listeners are not added/removed when a widget is created/destroyed
		   to notify their children & parents. We already do that as part of the normal process.
		WidgetVec::iterator itor = std::find( m_children.begin(), m_children.end(), widget );
		if( itor != m_children.end() )
			m_children.erase( itor );*/
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

		Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();
		Ogre::Vector2 relPos	= m_position * invCanvasSize2x - Ogre::Vector2::UNIT_SCALE;
		Ogre::Vector2 relSize	= m_size * invCanvasSize2x;

		m_derivedTopLeft		= mul( finalRot, relPos ) + parentPos;
		m_derivedBottomRight	= m_derivedTopLeft + mul( finalRot, relSize );
		m_derivedOrientation	= finalRot;

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_LOW
		m_transformOutOfDate = false;
#endif
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
	void Widget::setNextWidget( Widget * crystalgui_nullable nextWidget,
								Borders::Borders direction, bool reciprocate )
	{
		if( direction == Borders::NumBorders )
		{
			for( size_t i=0; i<Borders::NumBorders; ++i )
			{
				if( m_nextWidget[i] )
					m_nextWidget[i]->removeListener( this );
				m_nextWidget[i] = nextWidget;
				if( nextWidget )
				{
					nextWidget->addListener( this );
					if( reciprocate )
						nextWidget->setNextWidget( this, c_reciprocateBorders[i], false );
				}
			}
		}
		else
		{
			if( m_nextWidget[direction] )
				m_nextWidget[direction]->removeListener( this );
			m_nextWidget[direction] = nextWidget;
			if( nextWidget )
			{
				nextWidget->addListener( this );
				if( reciprocate )
					nextWidget->setNextWidget( this, c_reciprocateBorders[direction], false );
			}
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
	void Widget::broadcastNewVao( Ogre::VertexArrayObject *vao )
	{
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->broadcastNewVao( vao );
			++itor;
		}
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
	//-------------------------------------------------------------------------
	void Widget::setTransformDirty()
	{
#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_LOW
		m_transformOutOfDate = true;
#endif
	}
	//-------------------------------------------------------------------------
	void Widget::setTransform( const Ogre::Vector2 &position, const Ogre::Vector2 &size,
							   const Ogre::Matrix3 &orientation )
	{
		m_position = position;
		m_size = size;
		m_orientation = orientation;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setTransform( const Ogre::Vector2 &position, const Ogre::Vector2 &size )
	{
		m_position = position;
		m_size = size;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setPosition( const Ogre::Vector2 &position )
	{
		m_position = position;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setSize( const Ogre::Vector2 &size )
	{
		m_size = size;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setOrientation( const Ogre::Matrix3 &orientation )
	{
		m_orientation = orientation;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransformFromParent()
	{
		if( m_parent )
			m_parent->updateDerivedTransformFromParent();

		if( m_parent )
			updateDerivedTransform( m_parent->m_derivedTopLeft, m_parent->m_derivedOrientation );
		else
			updateDerivedTransform( Ogre::Vector2::ZERO, Ogre::Matrix3::IDENTITY );
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getDerivedTopLeft() const
	{
		CRYSTAL_ASSERT( !m_transformOutOfDate );
		return m_derivedTopLeft;
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getDerivedBottomRight() const
	{
		CRYSTAL_ASSERT( !m_transformOutOfDate );
		return m_derivedBottomRight;
	}
	//-------------------------------------------------------------------------
	const Ogre::Matrix3& Widget::getDerivedOrientation() const
	{
		CRYSTAL_ASSERT( !m_transformOutOfDate );
		return m_derivedOrientation;
	}
}
