
#include "ColibriGui/ColibriWidget.h"
#include "ColibriGui/ColibriWindow.h"

#include "ColibriGui/ColibriManager.h"

#define TODO_account_rotation

namespace Colibri
{
	static Borders::Borders c_reciprocateBorders[Borders::NumBorders+1u] =
	{
		Borders::Bottom,
		Borders::Right,
		Borders::Left,
		Borders::Top,
		Borders::NumBorders
	};

	const Matrix2x3 Matrix2x3::IDENTITY( 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );

	Widget::Widget( ColibriManager *manager ) :
		m_parent( 0 ),
		m_numNonRenderables( 0 ),
		m_numWidgets( 0 ),
		m_manager( manager ),
		m_hidden( false ),
		m_clickable( false ),
		m_keyboardNavigable( false ),
		m_childrenClickable( false ),
		m_pressable( true ),
		m_consumesScroll( false ),
		m_culled( false ),
		m_breadthFirst( false ),
		m_userId( 0 ),
		m_currentState( States::Idle ),
		m_position( Ogre::Vector2::ZERO ),
		m_size( Ogre::Vector2::ZERO ),
		m_orientation( Ogre::Vector4( 1.0f, 0.0f,  //
									  0.0f, 1.0f ) ),
		m_derivedTopLeft( Ogre::Vector2::ZERO ),
		m_derivedBottomRight( Ogre::Vector2::ZERO ),
		m_derivedOrientation( Matrix2x3::IDENTITY ),
		m_clipBorderTL( Ogre::Vector2::ZERO ),
		m_clipBorderBR( Ogre::Vector2::ZERO ),
		m_accumMinClipTL( -1.0f ),
		m_accumMaxClipBR( 1.0f ),
		m_zOrderDirty( false ),
		m_zOrderHasDirtyChildren( false ),
		m_zOrder( _wrapZOrderInternalId( 0 ) )
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		,
		m_transformOutOfDate( false ),
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
		COLIBRI_ASSERT( m_children.empty() && "_destroy not called before deleting!" );
	}
	//-------------------------------------------------------------------------
	size_t Widget::notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved )
	{
		size_t retVal = std::numeric_limits<size_t>::max();

		//Remove ourselves from being our parent's child
		WidgetVec::iterator itor = std::find( m_children.begin(), m_children.end(),
											  childWidgetBeingRemoved );

		COLIBRI_ASSERT_MEDIUM( itor != m_children.end() || m_destructionStarted );

		if( itor != m_children.end() )
		{
			//It may not be found if we're also in destruction phase
			retVal = static_cast<size_t>( itor - m_children.begin() );
			m_children.erase( itor );

			COLIBRI_ASSERT( (retVal < m_numWidgets && !childWidgetBeingRemoved->isWindow()) ||
							(retVal >= m_numWidgets && childWidgetBeingRemoved->isWindow()) );
			COLIBRI_ASSERT( (retVal < m_numNonRenderables && !childWidgetBeingRemoved->isRenderable()) ||
							(retVal >= m_numNonRenderables && childWidgetBeingRemoved->isRenderable()) );

			if( retVal < m_numNonRenderables )
				--m_numNonRenderables;

			if( retVal < m_numWidgets )
			{
				//This is a widget what we're removing
				--m_numWidgets;
			}
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Widget::setDebugName( const std::string &debugName )
	{
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		m_debugName = debugName;
#endif
	}
	//-------------------------------------------------------------------------
	const std::string &Widget::_getDebugName() const
	{
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		return m_debugName;
#else
		static std::string c_blankString = "";
		return c_blankString;
#endif
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
			WidgetVec::const_iterator end = children.begin() + ptrdiff_t( m_numWidgets );

			while( itor != end )
				m_manager->destroyWidget( *itor++ );

			//If 'this' is a ColibriWindow, then we should've already wiped out the windows.
			//Otherwise, then we shouldn't be having children windows!
			COLIBRI_ASSERT( m_numWidgets == children.size() );

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
		COLIBRI_ASSERT( !this->m_parent && "'this' already has a parent!" );
		COLIBRI_ASSERT( parent && "parent cannot be null!" );
		COLIBRI_ASSERT( (parent->isWindow() || thisIsWindow == parent->isWindow()) &&
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
		setTransformDirty( TransformDirtyPosition | TransformDirtyOrientation );
	}
	//-------------------------------------------------------------------------
	void Widget::setKeyboardFocus()
	{
		m_manager->_stealKeyboardFocus( this );
	}
	//-------------------------------------------------------------------------
	void Widget::setPressable( bool pressable )
	{
		m_pressable = pressable;
	}
	//-------------------------------------------------------------------------
	void Widget::_setDestructionDelayed() { m_hidden = true; }
	//-------------------------------------------------------------------------
	void Widget::setHidden( bool hidden )
	{
		if( m_hidden != hidden )
		{
			m_hidden = hidden;

			if( m_currentState != States::Idle )
			{
				setState( States::Idle );
				_callActionListeners( Action::Cancel );
			}

			if( m_keyboardNavigable )
				setWidgetNavigationDirty();
		}
	}
	//-------------------------------------------------------------------------
	Window * colibrigui_nullable Widget::getAsWindow()
	{
		if( isWindow() )
		{
			COLIBRI_ASSERT_HIGH( dynamic_cast<Window*>( this ) );
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

		COLIBRI_ASSERT_HIGH( dynamic_cast<Window*>( nextWidget ) );
		return static_cast<Window*>( nextWidget );
	}
	//-------------------------------------------------------------------------
	Widget * colibrigui_nullable Widget::getFirstKeyboardNavigableParent()
	{
		Widget *nextWidget = this;

		while( nextWidget->m_parent && !nextWidget->m_keyboardNavigable )
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
	Ogre::Vector2 Widget::mul( const Ogre::Vector4 &m2x2, Ogre::Vector2 xyPos )
	{
		Ogre::Vector2 result;
		result.x = m2x2.x * xyPos.x + m2x2.y * xyPos.y;
		result.y = m2x2.z * xyPos.x + m2x2.w * xyPos.y;
		return result;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::mul( const Matrix2x3 &mat, Ogre::Vector2 xyPos )
	{
		Ogre::Vector2 result;
		result.x = mat.m[0][0] * xyPos.x + mat.m[0][1] * xyPos.y + mat.m[0][2];
		result.y = mat.m[1][0] * xyPos.x + mat.m[1][1] * xyPos.y + mat.m[1][2];
		return result;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::mul( const Matrix2x3 &mat, float x, float y )
	{
		Ogre::Vector2 result;
		result.x = mat.m[0][0] * x + mat.m[0][1] * y + mat.m[0][2];
		result.y = mat.m[1][0] * x + mat.m[1][1] * y + mat.m[1][2];
		return result;
	}
	//-------------------------------------------------------------------------
	Matrix2x3 Widget::mul( const Matrix2x3 &a, const Matrix2x3 &b )
	{
		Matrix2x3 r;
		r.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0];
		r.m[0][1] = a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1];
		r.m[0][2] = a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2];

		r.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0];
		r.m[1][1] = a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1];
		r.m[1][2] = a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2];

		return r;
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransform( const Ogre::Vector2 &parentPos, const Matrix2x3 &parentRot )
	{
		const float invCanvasAr = m_manager->getCanvasInvAspectRatio();
		const Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();

		m_derivedTopLeft = parentPos + m_position * invCanvasSize2x;
		m_derivedBottomRight = m_derivedTopLeft + m_size * invCanvasSize2x;

		Ogre::Vector2 ndcCenter = ( m_derivedTopLeft + m_derivedBottomRight ) * 0.5f;
		ndcCenter.y *= invCanvasAr;
		const Ogre::Vector2 rotatedNdcCenter = mul( m_orientation, ndcCenter );

		const Ogre::Vector2 centerDiff = (ndcCenter - rotatedNdcCenter);

		m_derivedOrientation.m[0][0] = m_orientation.x;
		m_derivedOrientation.m[0][1] = m_orientation.y;
		m_derivedOrientation.m[0][2] = centerDiff.x;
		m_derivedOrientation.m[1][0] = m_orientation.z;
		m_derivedOrientation.m[1][1] = m_orientation.w;
		m_derivedOrientation.m[1][2] = centerDiff.y;

		m_derivedOrientation = mul( parentRot, m_derivedOrientation );

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
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
		COLIBRI_ASSERT( actionMask != 0 );
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
	void Widget::_callActionListeners( Action::Action action )
	{
		COLIBRI_ASSERT_HIGH(
			( action == Action::Cancel || m_manager->_isDelayingDestruction() ) &&
			"If listener->notifyWidgetAction destroys 'this', it will result in corruption" );

		const uint32_t actionMask = 1u << action;

		WidgetActionListenerRecordVec::const_iterator itor = m_actionListeners.begin();
		WidgetActionListenerRecordVec::const_iterator end  = m_actionListeners.end();

		while( itor != end )
		{
			if( itor->actionMask & actionMask )
				itor->listener->notifyWidgetAction( this, action );
			++itor;
		}

		if( action == Action::PrimaryActionPerform )
		{
			// Tell the window we were the last one to perform a primary action
			Widget *firstKeyboardNavigableParent = getFirstKeyboardNavigableParent();
			if( firstKeyboardNavigableParent )
			{
				Window *window = getFirstParentWindow();
				if( firstKeyboardNavigableParent->getParent() == window )
					window->setLastPrimaryAction( firstKeyboardNavigableParent );
			}
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
	void Widget::setClickable( bool bClickable )
	{
		if( m_clickable != bClickable )
		{
			m_clickable = bClickable;
			setWidgetNavigationDirty();
		}
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
		return m_keyboardNavigable && (!m_hidden && m_currentState != States::Disabled);
	}
	//-------------------------------------------------------------------------
	bool Widget::hasClickableChildren() const
	{
		return m_childrenClickable;
	}
	//-------------------------------------------------------------------------
	void Widget::setNextWidget( Widget *colibrigui_nullable nextWidget, Borders::Borders direction,
								bool reciprocate, bool bManualOverride )
	{
		if( direction == Borders::NumBorders )
		{
			for( size_t i=0; i<Borders::NumBorders; ++i )
			{
				if( m_nextWidget[i] )
					m_nextWidget[i]->removeListener( this );
				m_nextWidget[i] = nextWidget;
				m_autoSetNextWidget[i] = !bManualOverride;
				if( nextWidget )
				{
					nextWidget->addListener( this );
					if( reciprocate &&
						( bManualOverride || nextWidget->m_autoSetNextWidget[c_reciprocateBorders[i]] ) )
					{
						nextWidget->setNextWidget( this, c_reciprocateBorders[i], false,
												   bManualOverride );
					}
				}
			}
		}
		else
		{
			if( m_nextWidget[direction] )
				m_nextWidget[direction]->removeListener( this );
			m_nextWidget[direction] = nextWidget;
			m_autoSetNextWidget[direction] = !bManualOverride;
			if( nextWidget )
			{
				nextWidget->addListener( this );
				if( reciprocate && ( bManualOverride ||
									 nextWidget->m_autoSetNextWidget[c_reciprocateBorders[direction]] ) )
				{
					nextWidget->setNextWidget( this, c_reciprocateBorders[direction], false,
											   bManualOverride );
				}
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
		COLIBRI_ASSERT( this == child->m_parent );
		return !( 0.0f > child->m_position.x - currentScroll.x + child->m_size.x	||
				  0.0f > child->m_position.y - currentScroll.y + child->m_size.y	||
				  this->m_size.x < child->m_position.x - currentScroll.x			||
				  this->m_size.y < child->m_position.y - currentScroll.y );
	}
	//-------------------------------------------------------------------------
	bool Widget::intersects( const Ogre::Vector2 &posNdc ) const
	{
		COLIBRI_ASSERT_MEDIUM( !m_transformOutOfDate );
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

		while( ritor != rend && !retVal.widget && ( !retVal.window || !retVal.window->getClickable() ) )
		{
			retVal = ( *ritor )->_setIdleCursorMoved( newPosNdc );
			++ritor;
		}

		//One of the child windows is being touched by the cursor. We're done.
		if( retVal.widget || ( retVal.window && retVal.window->getClickable() ) )
			return retVal;

		if( !this->intersects( newPosNdc ) )
			return FocusPair();

		Ogre::Vector2 currentScroll = getCurrentScroll();

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.begin() + m_numWidgets;

		while( itor != end )
		{
			Widget *widget = *itor;
			if( (widget->m_clickable || widget->m_childrenClickable) &&
				!widget->isDisabled() &&
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
	void Widget::notifyCursorMoved( const Ogre::Vector2& posNDC )
	{
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
											  const Matrix2x3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );

		const Ogre::Vector2 invCanvasSize2x	= m_manager->getInvCanvasSize2x();
		const Ogre::Vector2 outerTopLeft	= this->m_derivedTopLeft;
		const Ogre::Vector2 outerTopLeftWithClipping = outerTopLeft + m_clipBorderTL * invCanvasSize2x;

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
	void Widget::_fillBuffersAndCommands( UiVertex ** RESTRICT_ALIAS vertexBuffer,
										  GlyphVertex ** RESTRICT_ALIAS textVertBuffer,
										  const Ogre::Vector2 &parentPos,
										  const Ogre::Vector2 &parentCurrentScrollPos,
										  const Matrix2x3 &parentRot )
	{
		updateDerivedTransform( parentPos, parentRot );

		m_culled = true;

		if( !m_parent->intersectsChild( this, parentCurrentScrollPos ) || m_hidden )
			return;

		m_culled = false;

		Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();

		Ogre::Vector2 parentDerivedTL = m_parent->m_derivedTopLeft +
										m_parent->m_clipBorderTL * invCanvasSize2x;
		Ogre::Vector2 parentDerivedBR = m_parent->m_derivedBottomRight -
										m_parent->m_clipBorderBR * invCanvasSize2x;

		parentDerivedTL.makeCeil( m_parent->m_accumMinClipTL );
		parentDerivedBR.makeFloor( m_parent->m_accumMaxClipBR );
		m_accumMinClipTL = parentDerivedTL;
		m_accumMaxClipBR = parentDerivedBR;

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		const Ogre::Vector2 currentScrollPos = Ogre::Vector2::ZERO /*getCurrentScroll()*/;
		const Ogre::Vector2 outerTopLeft = this->m_derivedTopLeft;
		const Ogre::Vector2 outerTopLeftWithClipping = outerTopLeft +
													   (m_clipBorderTL - currentScrollPos) *
													   invCanvasSize2x;

		while( itor != end )
		{
			(*itor)->_fillBuffersAndCommands( vertexBuffer, textVertBuffer,
											  outerTopLeftWithClipping, currentScrollPos,
											  m_derivedOrientation );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Widget::addNonRenderableCommands( ApiEncapsulatedObjects &apiObject,
										   bool collectingBreadthFirst )
	{
		if( m_culled )
			return;

		addChildrenCommands( apiObject, collectingBreadthFirst );
	}
	//-------------------------------------------------------------------------
	void Widget::addChildrenCommands( ApiEncapsulatedObjects &apiObject, bool collectingBreadthFirst )
	{
		if( !m_breadthFirst && !collectingBreadthFirst )
		{
			WidgetVec::const_iterator itor = m_children.begin();
			WidgetVec::const_iterator end  = m_children.begin() + m_numNonRenderables;

			while( itor != end )
			{
				(*itor)->addNonRenderableCommands( apiObject, false );
				++itor;
			}

			itor = m_children.begin() + m_numNonRenderables;
			end  = m_children.end();

			while( itor != end )
			{
				COLIBRI_ASSERT_HIGH( dynamic_cast<Renderable*>( *itor ) );
				Renderable *asRenderable = static_cast<Renderable*>( *itor );
				asRenderable->_addCommands( apiObject, false );
				++itor;
			}
		}
		else
		{
			WidgetVec *breadthFirst = m_manager->m_breadthFirst;

			breadthFirst[2].insert( breadthFirst[2].end(), m_children.begin(),
									m_children.begin() + m_numNonRenderables );
			breadthFirst[3].insert( breadthFirst[3].end(), m_children.begin() + m_numNonRenderables,
									m_children.end() );

			if( !collectingBreadthFirst )
			{
				//If we're here, this widget is the first in the hierarchy we found to be
				//using breadth first. So it's the one executing it and all children will
				//collect, and thus all children will use breadth first.
				//Siblings to this widget may be using depth first instead,
				//or may also become executers.
				while( !breadthFirst[2].empty() || !breadthFirst[3].empty() )
				{
					breadthFirst[0].swap( breadthFirst[2] );
					breadthFirst[1].swap( breadthFirst[3] );

					WidgetVec::const_iterator itor = breadthFirst[0].begin();
					WidgetVec::const_iterator end  = breadthFirst[0].end();

					while( itor != end )
					{
						(*itor)->addNonRenderableCommands( apiObject, true );
						++itor;
					}

					itor = breadthFirst[1].begin();
					end  = breadthFirst[1].end();

					while( itor != end )
					{
						COLIBRI_ASSERT_HIGH( dynamic_cast<Renderable*>( *itor ) );
						Renderable *asRenderable = static_cast<Renderable*>( *itor );
						asRenderable->_addCommands( apiObject, true );
						++itor;
					}

					breadthFirst[0].clear();
					breadthFirst[1].clear();
				}
			}
		}
	}
	//-------------------------------------------------------------------------
	bool Widget::isUltimatelyBreadthFirst() const
	{
		bool isBreadthFirst = m_breadthFirst;
		Widget const *nextParent = m_parent;

		while( !isBreadthFirst && nextParent )
		{
			isBreadthFirst = m_breadthFirst;
			nextParent = nextParent->m_parent;
		}

		return isBreadthFirst;
	}
	//-------------------------------------------------------------------------
	void Widget::setState( States::States state, bool smartHighlight, bool broadcastEnable )
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

		const States::States oldValue = m_currentState;

		m_currentState = state;

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			if( !(*itor)->isDisabled() || broadcastEnable )
				(*itor)->setState( state, false );
			++itor;
		}

		if( m_keyboardNavigable &&
			((state == States::Disabled && oldValue != States::Disabled) ||
			 (state != States::Disabled && oldValue == States::Disabled)) )
		{
			setWidgetNavigationDirty();
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
	size_t Widget::getOffsetStartWindowChildren() const
	{
		const size_t windowChildrenStart = m_numWidgets;
		COLIBRI_ASSERT_LOW( windowChildrenStart <= m_children.size() );
		return windowChildrenStart;
	}
	//-------------------------------------------------------------------------
	void Widget::setTransformDirty( uint32_t dirtyReason )
	{
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		m_transformOutOfDate = true;
#endif
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->setTransformDirty( TransformDirtyParentCaller | dirtyReason );
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Widget::scheduleSetTransformDirty()
	{
		m_manager->_scheduleSetTransformDirty( this );
	}
	//-------------------------------------------------------------------------
	void Widget::setTransform( const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size,
							   const Ogre::Vector4 &orientation )
	{
		m_position = topLeft;
		m_size = size;
		m_orientation = orientation;
		setTransformDirty( TransformDirtyAll );
	}
	//-------------------------------------------------------------------------
	void Widget::setTransform( const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size )
	{
		m_position = topLeft;
		m_size = size;
		setTransformDirty( TransformDirtyPosition | TransformDirtyScale );
	}
	//-------------------------------------------------------------------------
	void Widget::setZOrder( uint8_t z )
	{
		if( m_parent )
		{
			getParent()->m_zOrderDirty = true;
		}

		m_zOrder = _wrapZOrderInternalId( z );
		notifyZOrderChildWindowIsDirty( true );
		//The above function sets this to true in the case of recursive calls up the tree.
		//However from here we know no children should be set as dirty, so set it back to false.
		m_zOrderHasDirtyChildren = false;
	}
	//-------------------------------------------------------------------------
	uint16_t Widget::_wrapZOrderInternalId( uint8_t z ) const
	{
		uint16_t targetOrder = z;
		// Ensure renderables go after non-renderables
		if( isRenderable() )
			targetOrder |= 1u << 14u;
		// Ensure windows always go last
		if( isWindow() )
			targetOrder |= 1u << 15u;

		return targetOrder;
	}
	//-------------------------------------------------------------------------
	void Widget::notifyZOrderChildWindowIsDirty( bool firstCall )
	{
		m_zOrderHasDirtyChildren = true;

		if( m_parent )
		{
			Widget *parentWindow = getParent();
			parentWindow->notifyZOrderChildWindowIsDirty( false );
		}
		else
		{
			m_manager->_setZOrderWindowDirty( firstCall );
		}
	}
	//-------------------------------------------------------------------------
	void Widget::reorderWidgetVec( bool widgetInListDirty, WidgetVec& widgets )
	{
		if( widgetInListDirty )
		{
			std::stable_sort( widgets.begin(), widgets.end(), _compareWidgetZOrder );
		}

		WidgetVec::iterator itor = widgets.begin();
		WidgetVec::iterator endt = widgets.end();

		bool foundWindow = false;
		while( itor != endt )
		{
			// Sanity check that windows are placed at the end of the list.
			if( ( *itor )->isWindow() )
				foundWindow = true;
			if( foundWindow )
			{
				// From here on everything should be a window.
				COLIBRI_ASSERT_HIGH( ( *itor )->isWindow() );
			}

			if( ( *itor )->getZOrderHasDirtyChildren() )
			{
				( *itor )->updateZOrderDirty();
			}
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void Widget::updateZOrderDirty()
	{
		reorderWidgetVec( getZOrderDirty(), m_children );
		m_zOrderDirty = false;
		m_zOrderHasDirtyChildren = false;
	}
	//-------------------------------------------------------------------------
	void Widget::setTopLeft( const Ogre::Vector2 &topLeft )
	{
		m_position = topLeft;
		setTransformDirty( TransformDirtyPosition );
	}
	//-------------------------------------------------------------------------
	void Widget::setSize( const Ogre::Vector2 &size )
	{
		m_size = size;
		setTransformDirty( TransformDirtyScale );
	}
	//-------------------------------------------------------------------------
	void Widget::setOrientation( const Ogre::Vector4 &orientation )
	{
		m_orientation = orientation;
		setTransformDirty( TransformDirtyOrientation );
	}
	//-------------------------------------------------------------------------
	void Widget::setOrientation( const Ogre::Radian rotationAngle )
	{
		const float valueRadians = rotationAngle.valueRadians();
		m_orientation.x = std::cos( valueRadians );
		m_orientation.z = std::sin( valueRadians );
		m_orientation.y = -m_orientation.z;
		m_orientation.w = m_orientation.x;
		setTransformDirty( TransformDirtyOrientation );
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
		if( m_clipBorderTL.x != clipBorders[Borders::Left]	||
			m_clipBorderTL.y != clipBorders[Borders::Top]	||
			m_clipBorderBR.x != clipBorders[Borders::Right]	||
			m_clipBorderBR.y != clipBorders[Borders::Bottom] )
		{
			m_clipBorderTL.x = clipBorders[Borders::Left];
			m_clipBorderTL.y = clipBorders[Borders::Top];
			m_clipBorderBR.x = clipBorders[Borders::Right];
			m_clipBorderBR.y = clipBorders[Borders::Bottom];
			scheduleSetTransformDirty();
		}
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
	void Widget::setSizeAfterClipping( const Ogre::Vector2 &size )
	{
		setSize( size + (m_clipBorderTL + m_clipBorderBR) );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getSizeAfterClipping() const
	{
		return m_size - (m_clipBorderTL + m_clipBorderBR);
	}
	//-------------------------------------------------------------------------
	void Widget::updateDerivedTransformFromParent( bool updateParent )
	{
		if( m_parent )
		{
			if( updateParent )
				m_parent->updateDerivedTransformFromParent();

			Ogre::Vector2 currentScroll = m_parent->getCurrentScroll();
			const Ogre::Vector2 &invCanvasSize2x = m_manager->getInvCanvasSize2x();
			updateDerivedTransform( m_parent->m_derivedTopLeft +
									(m_parent->m_clipBorderTL - currentScroll) * invCanvasSize2x,
									m_parent->m_derivedOrientation );
		}
		else
		{
			//If we have no parent, then we're definitely a window
			updateDerivedTransform( -Ogre::Vector2::UNIT_SCALE, Matrix2x3::IDENTITY );
		}
	}
	//-------------------------------------------------------------------------
	void Widget::_setTextEdit( const char *text, int32_t selectStart, int32_t selectLength )
	{
	}
	//-------------------------------------------------------------------------
	void Widget::_setTextSpecialKey( uint32_t keyCode, uint16_t keyMod, size_t repetition )
	{
	}
	//-------------------------------------------------------------------------
	void Widget::_setTextInput( const char *text )
	{
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::_getImeLocation()
	{
		return Ogre::Vector2::ZERO;
	}
	//-------------------------------------------------------------------------
	bool Widget::isTextMultiline() const
	{
		return false;
	}
	//-------------------------------------------------------------------------
	bool Widget::wantsTextInput() const
	{
		return false;
	}
	//-------------------------------------------------------------------------
	void Widget::_update( float timeSinceLast )
	{
	}
	//-------------------------------------------------------------------------
	void Widget::_notifyCanvasChanged()
	{
		updateDerivedTransformFromParent( false );

		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.end();

		while( itor != end )
		{
			(*itor)->_notifyCanvasChanged();
			++itor;
		}
	}
	//-------------------------------------------------------------------------
	ColibriManager *Widget::getManager() { return m_manager; }
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getDerivedTopLeft() const
	{
		COLIBRI_ASSERT_MEDIUM( !m_transformOutOfDate );
		return m_derivedTopLeft;
	}
	//-------------------------------------------------------------------------
	const Ogre::Vector2& Widget::getDerivedBottomRight() const
	{
		COLIBRI_ASSERT_MEDIUM( !m_transformOutOfDate );
		return m_derivedBottomRight;
	}
	//-------------------------------------------------------------------------
	const Matrix2x3 &Widget::getDerivedOrientation() const
	{
		COLIBRI_ASSERT_MEDIUM( !m_transformOutOfDate );
		return m_derivedOrientation;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getDerivedCenter() const
	{
		COLIBRI_ASSERT_MEDIUM( !m_transformOutOfDate );
		return (m_derivedTopLeft + m_derivedBottomRight) * 0.5f;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::calculateChildrenSize() const
	{
		Ogre::Vector2 maxSize( Ogre::Vector2::ZERO );
		WidgetVec::const_iterator itor = m_children.begin();
		WidgetVec::const_iterator end  = m_children.begin() + getOffsetStartWindowChildren();

		while( itor != end )
		{
			Widget *widget = *itor;
			if( !widget->isHidden() )
				maxSize.makeCeil( widget->getLocalTopLeft() + widget->getSize() );
			++itor;
		}

		return maxSize;
	}
	//-------------------------------------------------------------------------
	void Widget::setSizeAndCellMinSize( const Ogre::Vector2 &size )
	{
		setSize( size );
		m_minSize = size;
	}
	//-------------------------------------------------------------------------
	void Widget::setCellOffset( const Ogre::Vector2 &topLeft )
	{
		setTopLeft( topLeft );
	}
	//-------------------------------------------------------------------------
	void Widget::setCellSize( const Ogre::Vector2 &size )
	{
		setSize( size );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getCellSize() const
	{
		return m_size;
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Widget::getCellMinSize() const
	{
		Ogre::Vector2 retVal( m_minSize );
		retVal.makeCeil( calculateChildrenSize() + getBorderCombined() );
		return retVal;
	}
}
