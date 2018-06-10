
#include "CrystalGui/CrystalWidget.h"
#include "CrystalGui/CrystalWindow.h"

#include "CrystalGui/CrystalManager.h"

#define TODO_account_rotation

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
		m_numNonRenderables( 0 ),
		m_numWidgets( 0 ),
		m_manager( manager ),
		m_hidden( false ),
		m_clickable( false ),
		m_keyboardNavigable( false ),
		m_childrenClickable( false ),
		m_pressable( true ),
		m_currentState( States::Idle ),
		m_position( Ogre::Vector2::ZERO ),
		m_size( manager->getCanvasSize() ),
		m_orientation( Ogre::Matrix3::IDENTITY ),
		m_derivedTopLeft( Ogre::Vector2::ZERO ),
		m_derivedBottomRight( Ogre::Vector2::ZERO ),
		m_derivedOrientation( Ogre::Matrix3::IDENTITY ),
		m_clipBorderTL( Ogre::Vector2::ZERO ),
		m_clipBorderBR( Ogre::Vector2::ZERO ),
		m_accumMinClipTL( -1.0f ),
		m_accumMaxClipBR( 1.0f )
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

			CRYSTAL_ASSERT( (retVal < m_numWidgets && !childWidgetBeingRemoved->isWindow()) ||
							(retVal >= m_numWidgets && childWidgetBeingRemoved->isWindow()) );
			CRYSTAL_ASSERT( (retVal < m_numNonRenderables && !childWidgetBeingRemoved->isRenderable()) ||
							(retVal >= m_numNonRenderables && childWidgetBeingRemoved->isRenderable()) );

			if( retVal < m_numNonRenderables )
				--m_numNonRenderables;

			if( retVal < m_numWidgets )
			{
				//This is a widget what we're removing
				--m_numWidgets;
			}
		}

		CRYSTAL_ASSERT_MEDIUM( itor != m_children.end() || m_destructionStarted );

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Widget::_initialize()
	{
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
			size_t idx = parent->m_numWidgets;
			if( !this->isRenderable() )
			{
				idx = parent->m_numNonRenderables;
				++parent->m_numNonRenderables;
			}
			parent->m_children.insert( parent->m_children.begin() + idx, this );
			++parent->m_numWidgets; //Must be incremented regardless of whether it's a renderable
		}
		else
		{
			parent->m_children.push_back( this );
		}
		parent->setWidgetNavigationDirty();
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setPressable( bool pressable )
	{
		m_pressable = pressable;
	}
	//-------------------------------------------------------------------------
	Window * crystalgui_nullable Widget::getAsWindow()
	{
		if( isWindow() )
		{
			CRYSTAL_ASSERT_HIGH( dynamic_cast<Window*>( this ) );
			return static_cast<Window*>( this );
		}

		return 0;
	}
	//-------------------------------------------------------------------------
	Window* Widget::getFirstParentWindow()
	{
		Widget *nextWidget = this;

		while( !nextWidget->isWindow() )
			nextWidget = nextWidget->m_parent;

		CRYSTAL_ASSERT_HIGH( dynamic_cast<Window*>( nextWidget ) );
		return static_cast<Window*>( nextWidget );
	}
	//-------------------------------------------------------------------------
	Widget * crystalgui_nullable Widget::getFirstKeyboardNavigableParent()
	{
		Widget *nextWidget = this;

		while( !nextWidget->m_keyboardNavigable )
			nextWidget = nextWidget->m_parent;

		return nextWidget;
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getCurrentScroll() const
	{
		return Ogre::Vector2::ZERO;
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
	void Widget::addActionListener( WidgetActionListener *listener, uint32_t actionMask )
	{
		CRYSTAL_ASSERT( actionMask != 0 );
		WidgetActionListenerRecordVec::iterator itor = m_actionListeners.begin();
		WidgetActionListenerRecordVec::iterator end  = m_actionListeners.end();

		while( itor != end && itor->listener != listener )
			++itor;

		if( itor == end )
			m_actionListeners.push_back( WidgetActionListenerRecord( actionMask, listener ) );
		else
			itor->actionMask |= actionMask;
	}
	//-------------------------------------------------------------------------
	void Widget::removeActionListener( WidgetActionListener *listener, uint32_t actionMask )
	{
		WidgetActionListenerRecordVec::iterator itor = m_actionListeners.begin();
		WidgetActionListenerRecordVec::iterator end  = m_actionListeners.end();

		while( itor != end && itor->listener != listener )
			++itor;

		if( itor != end )
		{
			itor->actionMask &= ~actionMask;
			//Preserve the order in which listeners were added
			if( !itor->actionMask )
				m_actionListeners.erase( itor );
		}
		else
		{
			LogListener	*log = m_manager->getLogListener();
			log->log( "Widget::removeListener could not find WidgetActionListener. Ignoring",
					  LogSeverity::Warning );
		}
	}
	//-------------------------------------------------------------------------
	void Widget::callActionListeners( Action::Action action )
	{
		const uint32_t actionMask = 1u << action;

		WidgetActionListenerRecordVec::const_iterator itor = m_actionListeners.begin();
		WidgetActionListenerRecordVec::const_iterator end  = m_actionListeners.end();

		while( itor != end )
		{
			if( itor->actionMask & actionMask )
				itor->listener->notifyWidgetAction( this, action );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Widget::_notifyActionKeyMovement( Borders::Borders direction )
	{
	}
	//-------------------------------------------------------------------------
	void Widget::setWidgetNavigationDirty()
	{
		m_parent->setWidgetNavigationDirty();
	}
	//-------------------------------------------------------------------------
	void Widget::setKeyboardNavigable( bool bNavigable )
	{
		if( m_keyboardNavigable != bNavigable )
		{
			m_keyboardNavigable = bNavigable;
			setWidgetNavigationDirty();
		}
	}
	//-------------------------------------------------------------------------
	bool Widget::isKeyboardNavigable() const
	{
		return m_keyboardNavigable;
	}
	//-------------------------------------------------------------------------
	bool Widget::hasClickableChildren() const
	{
		return m_childrenClickable;
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
					if( reciprocate && nextWidget->m_autoSetNextWidget[c_reciprocateBorders[i]] )
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
				if( reciprocate && nextWidget->m_autoSetNextWidget[c_reciprocateBorders[direction]] )
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
	bool Widget::intersectsChild( Widget *child, const Ogre::Vector2 &currentScroll ) const
	{
		CRYSTAL_ASSERT( this == child->m_parent );
		return !( 0.0f > child->m_position.x - currentScroll.x + child->m_size.x	||
				  0.0f > child->m_position.y - currentScroll.y + child->m_size.y	||
				  this->m_size.x < child->m_position.x - currentScroll.x			||
				  this->m_size.y < child->m_position.y - currentScroll.y );
	}
	//-------------------------------------------------------------------------
	bool Widget::intersects( const Ogre::Vector2 &posNdc ) const
	{
		CRYSTAL_ASSERT_MEDIUM( !m_transformOutOfDate );
		TODO_account_rotation;
		return !( posNdc.x < m_derivedTopLeft.x ||
				  posNdc.y < m_derivedTopLeft.y ||
				  posNdc.x > m_derivedBottomRight.x ||
				  posNdc.y > m_derivedBottomRight.y );
	}
	//-------------------------------------------------------------------------
	FocusPair Widget::_setIdleCursorMoved( const Ogre::Vector2 &newPosNdc )
	{
		FocusPair retVal;

		//The first window that our button is touching wins. We go in LIFO order.
		const size_t numWindows = m_children.size() - m_numWidgets;
		WidgetVec::const_reverse_iterator ritor = m_children.rbegin();
		WidgetVec::const_reverse_iterator rend  = m_children.rbegin() + numWindows;

		while( ritor != rend && !retVal.widget )
		{
			retVal = (*ritor)->_setIdleCursorMoved( newPosNdc );
			++ritor;
		}

		//One of the child windows is being touched by the cursor. We're done.
		if( retVal.widget )
			return retVal;

		if( !this->intersects( newPosNdc ) )
			return FocusPair();

		Ogre::Vector2 currentScroll = getCurrentScroll();

		WidgetVec::const_iterator itor = m_children.begin() + m_numNonRenderables;
		WidgetVec::const_iterator end  = m_children.begin() + m_numWidgets;

		while( itor != end )
		{
			Widget *widget = *itor;
			if( (widget->m_clickable || widget->m_childrenClickable) &&
				this->intersectsChild( widget, currentScroll ) &&
				widget->intersects( newPosNdc ) )
			{
				if( widget->m_clickable )
					retVal.widget = widget;

				if( widget->m_childrenClickable )
				{
					FocusPair childFocusPair;
					childFocusPair = widget->_setIdleCursorMoved( newPosNdc );
					if( childFocusPair.widget )
						retVal = childFocusPair;
				}
			}
			++itor;
		}

		retVal.window = getFirstParentWindow();

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Widget::broadcastNewVao( Ogre::VertexArrayObject *vao, Ogre::VertexArrayObject *textVao )
	{
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->broadcastNewVao( vao, textVao );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Widget::_updateDerivedTransformOnly( const Ogre::Vector2 &parentPos,
											  const Ogre::Matrix3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );

		const Ogre::Vector2 invCanvasSize2x	= m_manager->getInvCanvasSize2x();
		const Ogre::Vector2 outerTopLeft	= this->m_derivedTopLeft;
		const Ogre::Vector2 outerTopLeftWithClipping = outerTopLeft + m_clipBorderTL * invCanvasSize2x;

		const Ogre::Matrix3 finalRot = this->m_derivedOrientation;
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->_updateDerivedTransformOnly( outerTopLeftWithClipping, finalRot );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Widget::_fillBuffersAndCommands( UiVertex ** RESTRICT_ALIAS vertexBuffer,
										  GlyphVertex ** RESTRICT_ALIAS textVertBuffer,
										  const Ogre::Vector2 &parentPos,
										  const Ogre::Vector2 &parentCurrentScrollPos,
										  const Ogre::Matrix3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );
	}
	//-------------------------------------------------------------------------
	void Widget::setState( States::States state, bool smartHighlight )
	{
		if( isWindow() )
			return;

		if( smartHighlight &&
			((m_currentState == States::HighlightedButtonAndCursor &&
			  (state == States::HighlightedCursor || state == States::HighlightedButton)) ||
			 (state == States::HighlightedCursor && m_currentState == States::HighlightedButton) ||
			 (state == States::HighlightedButton && m_currentState == States::HighlightedCursor)) )
		{
			state = States::HighlightedButtonAndCursor;
		}

		m_currentState = state;

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->setState( state, false );
			++itor;
		}
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
	void Widget::setClipBorders( float clipBorders[Borders::NumBorders] )
	{
		m_clipBorderTL.x = clipBorders[Borders::Left];
		m_clipBorderTL.y = clipBorders[Borders::Top];
		m_clipBorderBR.x = clipBorders[Borders::Right];
		m_clipBorderBR.y = clipBorders[Borders::Bottom];
		setTransformDirty();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getTopLeftAfterClipping() const
	{
		return m_position + m_clipBorderTL;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getBottomRightAfterClipping() const
	{
		return m_position + m_size - m_clipBorderBR;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getSizeAfterClipping() const
	{
		return m_size - (m_clipBorderTL + m_clipBorderBR);
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransformFromParent()
	{
		if( m_parent )
		{
			m_parent->updateDerivedTransformFromParent();

			Ogre::Vector2 currentScroll = m_parent->getCurrentScroll();
			const Ogre::Vector2 &invCanvasSize2x = m_manager->getInvCanvasSize2x();
			updateDerivedTransform( m_parent->m_derivedTopLeft +
									(m_parent->m_clipBorderTL - currentScroll) * invCanvasSize2x,
									m_parent->m_derivedOrientation );
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
