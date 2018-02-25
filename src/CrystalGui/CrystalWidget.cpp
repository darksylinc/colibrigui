
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
		m_numWidgets( 0 ),
		m_manager( manager ),
		m_hidden( false ),
		m_currentState( States::Idle ),
		m_position( Ogre::Vector2::ZERO ),
		m_size( manager->getCanvasSize() ),
		m_orientation( Ogre::Matrix3::IDENTITY ),
		m_derivedTopLeft( Ogre::Vector2::ZERO ),
		m_derivedBottomRight( Ogre::Vector2::ZERO ),
		m_derivedOrientation( Ogre::Matrix3::IDENTITY )
  #if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_MEDIUM
	,	m_transformOutOfDate( false ),
		m_destructionStarted( false )
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
		size_t retVal = std::numeric_limits<size_t>::max();

		//Remove ourselves from being our parent's child
		WidgetVec::iterator itor = std::find( m_children.begin(), m_children.end(),
											  childWidgetBeingRemoved );
		if( itor != m_children.end() )
		{
			//It may not be found if we're also in destruction phase
			retVal = itor - m_children.begin();
			m_children.erase( itor );

			CRYSTAL_ASSERT( retVal < m_numWidgets && !childWidgetBeingRemoved->isWindow() ||
							retVal >= m_numWidgets && childWidgetBeingRemoved->isWindow() );

			if( retVal < m_numWidgets )
			{
				//This is a widget what we're removing
				++m_numWidgets;
			}
		}

		CRYSTAL_ASSERT_MEDIUM( itor != m_children.end() || m_destructionStarted );

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Widget::_destroy()
	{
		m_destructionStarted = true;

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
			WidgetVec::const_iterator end  = children.begin() + m_numWidgets;

			while( itor != end )
				m_manager->destroyWidget( *itor++ );

			//If 'this' is a CrystalWindow, then we should've already wiped out the windows.
			//Otherwise, then we shouldn't be having children windows!
			CRYSTAL_ASSERT( m_numWidgets == children.size() );

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

		m_destructionStarted = false;
	}
	//-------------------------------------------------------------------------
	void Widget::_setParent( Widget *parent )
	{
		const bool thisIsWindow = this->isWindow();
		CRYSTAL_ASSERT( !this->m_parent && "'this' already has a parent!" );
		CRYSTAL_ASSERT( parent && "parent cannot be null!" );
		CRYSTAL_ASSERT( (parent->isWindow() || thisIsWindow == parent->isWindow()) &&
						"Regular Widgets cannot be parents of windows!" );
		this->m_parent = parent;
		if( !thisIsWindow )
		{
			parent->m_children.insert( parent->m_children.begin() + parent->m_numWidgets, this );
			++parent->m_numWidgets;
		}
		else
		{
			parent->m_children.push_back( this );
		}
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

		Ogre::Vector2 center = getCenter();

		Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();
		Ogre::Vector2 relCenter		= center * invCanvasSize2x;

		Ogre::Vector2 derivedHalfSize	= mul( finalRot, m_size * 0.5f * invCanvasSize2x );
		Ogre::Vector2 derivedCenter		= mul( finalRot, relCenter ) + parentPos;

		m_derivedTopLeft		= derivedCenter - derivedHalfSize;
		m_derivedBottomRight	= derivedCenter + derivedHalfSize;
		m_derivedOrientation	= finalRot;

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_MEDIUM
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
	bool Widget::intersectsChild( Widget *child ) const
	{
		CRYSTAL_ASSERT( this == child->m_parent );
		return !( 0.0f > child->m_position.x + child->m_size.x	||
				  0.0f > child->m_position.y + child->m_size.y	||
				  this->m_size.x < child->m_position.x	||
				  this->m_size.y < child->m_position.y );
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
#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_MEDIUM
		m_transformOutOfDate = true;

	#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_HIGH
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->setTransformDirty();
			++itor;
		}
	#endif
#endif
	}
	//-------------------------------------------------------------------------
	void Widget::setTransform( const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size,
							   const Ogre::Matrix3 &orientation )
	{
		m_position = topLeft;
		m_size = size;
		m_orientation = orientation;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setTransform( const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size )
	{
		m_position = topLeft;
		m_size = size;
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setTopLeft( const Ogre::Vector2 &topLeft )
	{
		m_position = topLeft;
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
	void Widget::setCenter( const Ogre::Vector2 &center )
	{
		setTopLeft( center - this->m_size * 0.5f );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getCenter() const
	{
		return m_position + this->m_size * 0.5f;
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransformFromParent()
	{
		if( m_parent )
		{
			m_parent->updateDerivedTransformFromParent();
			updateDerivedTransform( m_parent->m_derivedTopLeft, m_parent->m_derivedOrientation );
		}
		else
			updateDerivedTransform( Ogre::Vector2::ZERO, Ogre::Matrix3::IDENTITY );
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getDerivedTopLeft() const
	{
		CRYSTAL_ASSERT_MEDIUM( !m_transformOutOfDate );
		return m_derivedTopLeft;
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getDerivedBottomRight() const
	{
		CRYSTAL_ASSERT_MEDIUM( !m_transformOutOfDate );
		return m_derivedBottomRight;
	}
	//-------------------------------------------------------------------------
	const Ogre::Matrix3& Widget::getDerivedOrientation() const
	{
		CRYSTAL_ASSERT_MEDIUM( !m_transformOutOfDate );
		return m_derivedOrientation;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getDerivedCenter() const
	{
		CRYSTAL_ASSERT_MEDIUM( !m_transformOutOfDate );
		return (m_derivedTopLeft + m_derivedBottomRight) * 0.5f;
	}
}
