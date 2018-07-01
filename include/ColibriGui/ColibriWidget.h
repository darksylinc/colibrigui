
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "OgreVector2.h"
#include "OgreMatrix3.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	struct UiVertex;
	struct GlyphVertex;
	struct ApiEncapsulatedObjects;
	typedef std::vector<Widget*> WidgetVec;
	typedef std::vector<Window*> WindowVec;

	class WidgetActionListener
	{
	public:
		virtual void notifyWidgetAction( Widget *widget, Action::Action action ) = 0;
	};

	class WidgetListener
	{
	public:
		virtual void notifyWidgetDestroyed( Widget *widget ) {}
	};

	struct WidgetListenerPair
	{
		size_t			refCount;
		WidgetListener	*listener;

		WidgetListenerPair( WidgetListener *_listener ) :
			refCount( 0 ), listener( _listener ) {}
	};

	typedef std::vector<WidgetListenerPair> WidgetListenerPairVec;

	class Widget : public WidgetListener
	{
	protected:
		friend class ColibriManager;
		friend class Renderable;
		friend class Label;

		struct WidgetActionListenerRecord
		{
			uint32_t				actionMask;
			WidgetActionListener	*listener;
			WidgetActionListenerRecord( uint32_t _actionMask, WidgetActionListener *_listener ) :
				actionMask( _actionMask ), listener( _listener ) {}
		};

		typedef std::vector<WidgetActionListenerRecord> WidgetActionListenerRecordVec;

		/// Child and parent relationships are explicit, thus they're not tracked via
		/// WidgetListener::notifyWidgetDestroyed
		/// Can only be nullptr if 'this' is a Window.
		/// Windows can only be children of Windows.
		Widget * colibrigui_nonnull m_parent;
		/// Includes windows, and they're always at the end. See m_numWidgets
		WidgetVec					m_children;
		/// Widgets that are Renderables start from m_children[m_numNonRenderables]
		size_t						m_numNonRenderables;
		/// Windows stored in m_children start from m_children[m_numWidgets]
		/// The ranges are:
		/// [0; m_numNonRenderables)			-> Widgets, not renderables
		/// [m_numNonRenderables; m_numWidgets)	-> Widgets that are renderables
		/// [m_numWidgets; m_children.size())	-> Windows
		size_t						m_numWidgets;

		ColibriManager          *m_manager;

		Widget * colibrigui_nullable	m_nextWidget[Borders::NumBorders];
		bool							m_autoSetNextWidget[Borders::NumBorders];
		bool					m_hidden;
		/// Can be highlighted & pressed by mouse cursor
		bool					m_clickable;
		/// Can be highlighted & pressed by keyboard
		bool					m_keyboardNavigable;
		/// Widget has children which may be clickable, but are not navigable via keyboard
		bool					m_childrenClickable;
		/// Whether this widget can go into States::Pressed state via keyboard or mouse cursor.
		/// (assuming it is either clickable or keyboard navigable, otherwise it would be impossible)
		bool					m_pressable;

		bool m_culled;
	public:
		/// When true, this widgets and its children will be rendered in breadth first
		/// order, instead of depth first.
		/// Breadth first can result in incorrect rendering if two sibling or children
		/// widgets overlap, but can result in a significant performance boost in most
		/// common scenarios
		///
		/// This performance boost comes from the fact that sibling widgets commonly
		/// use the same skin / materials, but their children break it.
		///
		/// For example almost all Buttons are the same, but if Buttons have text in
		/// it, with depth-first the text must be rendered between each button.
		/// This breaks batching and causes the number_buttons*2 draw calls and
		/// shader switching.
		///
		/// By using breadth first instead, all buttons are rendered together, and then
		/// their text. This causes just 2 draw calls to be issued, regardless of the
		/// number of buttons, thus it can cause a tremendous performance boost.
		///
		/// Because breadth first rendering can cause rendering artifacts if the widgets
		/// overlaps (because child Widgets may be drawn on top of a sibling Widget,
		/// instead of behind), you can control the rendering mode per widget.
		///
		/// Note that it is the parent who must be set m_breadthFirst to true.
		/// In the example above, it is pointless to set the Buttons or the text's
		/// m_breadthFirst. You must set the parent window's m_breadthFirst of
		/// all those buttons in order to work.
		///
		/// @remark	PUBLIC MEMEBER: CAN BE EDITED DIRECTLY
		/// @remark	This value affects children. If this->m_breadthFirst is set to
		///			false, it may still be drawn as breadth first if any of our
		///			parents has this value set to true
		///
		/// @see	Widget::addChildrenCommands
		/// @see	Widget::isUltimatelyBreadthFirst
		bool m_breadthFirst;

	protected:
		States::States			m_currentState;

		WidgetListenerPairVec			m_listeners;
		WidgetActionListenerRecordVec	m_actionListeners;

		Ogre::Vector2	m_position;
		Ogre::Vector2	m_size;
		Ogre::Matrix3	m_orientation;

		Ogre::Vector2	m_derivedTopLeft;
		Ogre::Vector2	m_derivedBottomRight;
		Ogre::Matrix3	m_derivedOrientation;

		Ogre::Vector2	m_clipBorderTL;
		Ogre::Vector2	m_clipBorderBR;

		/// Without m_accumMinClipTL & m_accumMaxClipBR, 'this' will be visible
		/// even if our parent is partially obscured by our parent's parent
		Ogre::Vector2	m_accumMinClipTL;
		Ogre::Vector2	m_accumMaxClipBR;

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		bool	m_transformOutOfDate;
		bool	m_destructionStarted;
#endif

		WidgetListenerPairVec::iterator findListener( WidgetListener *listener );

		static Ogre::Vector2 mul( const Ogre::Matrix3 &matrix, Ogre::Vector2 xyPos );

		void updateDerivedTransform( const Ogre::Vector2 &parentPos, const Ogre::Matrix3 &parentRot );

		/** Notifies a parent that the input is about to be removed. It's similar to
			notifyWidgetDestroyed, except this is explicitly about child-parent
			relationships, as these relationships aren't tracked by listeners.
		@return
			Index to m_children where childWidgetBeingRemoved used to be.
		*/
		virtual size_t notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved );

		virtual void stateChanged( States::States newState ) {}

		virtual void setTransformDirty();
		void scheduleSetTransformDirty();

	public:
		Widget( ColibriManager *manager );
		virtual ~Widget();

		virtual void _initialize();
		virtual void _destroy();

		/// Do not call directly. 'this' cannot be a Window
		void _setParent( Widget *parent );
		Widget * colibrigui_nonnull getParent() const				{ return m_parent; }

		virtual bool isRenderable() const	{ return false; }
		virtual bool isWindow() const		{ return false; }
		virtual bool isLabel() const		{ return false; }

		void setPressable( bool pressable );
		bool isPressable() const			{ return m_pressable; }

		void setHidden( bool hidden );
		bool isHidden() const				{ return m_hidden; }

		bool isDisabled() const				{ return m_currentState == States::Disabled; }

		/// If 'this' is a window, it returns 'this'. Otherwise it returns nullptr
		Window * colibrigui_nullable getAsWindow();

		/// If 'this' is a window, it returns 'this'. Otherwise it returns its
		/// parent (or its parent's parent) until we find a window. Cannot
		/// return null
		Window* getFirstParentWindow();

		/// Will return the first Widget in the hierarchy where m_keyboardNavigable == true
		/// Note it can return nullptr if no widget matches the criteria.
		/// Note it can return 'this'
		Widget * colibrigui_nullable getFirstKeyboardNavigableParent();

		/// See Window::getCurrentScroll. For most widgets, this returns
		/// zero (i.e. when scrolling is not supported)
		virtual const Ogre::Vector2& getCurrentScroll() const;

		// WidgetListener overload
		virtual void notifyWidgetDestroyed( Widget *widget );

		void addListener( WidgetListener *listener );
		void removeListener( WidgetListener *listener );

		/** Adds a listener to listen for certain events such as when
			the Widget is highlighted or pressed.

			Insertion order is preserved. Note however if you later add more actions to an
			already registered listener, these actions will be joined. For example:

				addListener( listenerA, ActionMask::Highlighted );
				addListener( listenerB, ActionMask::Cancel );
				addListener( listenerA, ActionMask::Cancel );

			For the event "ActionMask::Cancel", Listener A will be called before B, because
			A was registered before B even though it was for the event "Highlighted".
		@param listener
			Listener to receive the callbacks.
		@param actionMask
			See ActionMask::ActionMask. The mask defines what events will be notified.
			Cannot be 0.
		*/
		void addActionListener( WidgetActionListener *listener, uint32_t actionMask=~0u );

		/** Remove a registered listener. Does nothing but issue a warning if listener does
			not exist.

			The listener isn't actually removed until all bits from actionMask are unset.
		@param listener
			Listener already registered with addListener
		@param actionMask
			See ActionMask::ActionMask. The mask to remove.
		*/
		void removeActionListener( WidgetActionListener *listener, uint32_t actionMask=~0u );

		void callActionListeners( Action::Action action );

		/** Gets called when user hits a direction with the keyboard, and there's no next widget
			to go to (thus we capture that and may be interpreted as an action. eg. Spinners use this)
		@param direction
		*/
		virtual void _notifyActionKeyMovement( Borders::Borders direction );

		/** Call this to indicate the widget has been moved or created; thus we'll broadcast
			the message until reaching our most immediate parent window.
			This window is now aware we've changed, and will later recalculate all
			the connections of its children.
			See setNextWidget
		*/
		virtual void setWidgetNavigationDirty();

		/** When false, this widget cannot be highlighted or pressed via Keyboard.
			It's similar to being disabled, except it's faster in CPU terms, and disabled objects
			change their skin; whereas you can explicitly change the state without the user being
			able to hit or highlight this widget with the HIDs (keyboard, mouse, etc).

			For example a Label may not be navigable, but you still want to toggle between Idle and
			Disabled states based on other criteria, thus visually graying out the text while
			disabled.

			Default: Depends on the Widget. Base class defaults to false.
			Derived classes may override it.

			Note: This implicitly calls setWidgetNavigationDirty
		@param bNavigable
		*/
		void setKeyboardNavigable( bool bNavigable );
		/// Returns m_keyboardNavigable. See isKeyboardNavigable
		bool getKeyboardNavigable() const				{ return m_keyboardNavigable; }
		/// Returns whether the widget is actually navigable
		bool isKeyboardNavigable() const;

		/// @copydoc m_childrenClickable
		bool hasClickableChildren() const;

		/** Sets the next widget to go to. For example if calling
				a->setNextWidget( b, Borders::Right, true );
			then when we're at 'a' and user hits the right button, we will switch to 'b'.
			And if we're at 'b' and user hits left button, we will switch to 'a' because
			reciprocate was set to true.
		@param nextWidget
		@param direction
		@param reciprocate
			Whether the other widget should also be set to target us in the opposite direction.
		*/
		void setNextWidget( Widget * colibrigui_nullable nextWidget,
							Borders::Borders direction, bool reciprocate=true );

		float getRight() const;
		float getBottom() const;

		/**
		@return
			True if the given widget is partially intersecting or fully inside this, or viceversa.
		*/
		bool intersectsChild( Widget *child, const Ogre::Vector2 &currentScroll ) const;
		//bool intersects( Widget *widget ) const;

		/// Input must be in NDC space i.e. in range [-1; 1]
		bool intersects( const Ogre::Vector2 &posNdc ) const;

		FocusPair _setIdleCursorMoved( const Ogre::Vector2 &newPosNdc );

		virtual void broadcastNewVao( Ogre::VertexArrayObject *vao, Ogre::VertexArrayObject *textVao );

		virtual void _updateDerivedTransformOnly( const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );

		/** Fills vertexBuffer & textVertBuffer for rendering, perfoming occlussion culling.
			It also updates derived transforms. Derived classes change their functionality.
			This function is mostly relevant in Renderable and its derived classes
		@param vertexBuffer
			Filled by most Renderable classes.
		@param textVertBuffer
			Only filled by Label
		@param parentPos
			Derived position m_parent, in clip space. If the parent supports scrolling (e.g. windows)
			then this position is already offsetted by the scroll.
			Implementations should not try to implement parentCurrentScrollPos twice!
		@param parentCurrentScrollPos
			Amount of scroll from m_parent, in canvas space.
			This is required needed so we can perform certain computations.
		@param parentRot
			Derived orientation of m_parent
		*/
		virtual void _fillBuffersAndCommands( UiVertex * colibrigui_nonnull * colibrigui_nonnull
											  RESTRICT_ALIAS vertexBuffer,
											  GlyphVertex * colibrigui_nonnull * colibrigui_nonnull
											  RESTRICT_ALIAS textVertBuffer,
											  const Ogre::Vector2 &parentPos,
											  const Ogre::Vector2 &parentCurrentScrollPos,
											  const Ogre::Matrix3 &parentRot );
	protected:
		void addNonRenderableCommands( ApiEncapsulatedObjects &apiObject, bool collectingBreadthFirst );

		/** There are 3 rendering modes we can idenfity:
			1. Depth First. This guarantees Widgets are rendered in correct order.
			2. Executor Breadth First. When this->m_breadthFirst is set, this widget
			   and ALL of its children will render in breadth first mode (even if our
			   children have m_breadthFirst unset).
			   This widget will also perform rendering commands.
			3. Collecter breadth first. When our parent (or our parent's parent parent...)
			   has m_breadthFirst set, our value of m_breadthFirst does not matter anymore
			   and we're "collecters", that means we just inform our parents what Widgets
			   are our children, so the Executor can render them

			@see	Widget::m_breadthFirst
			@see	Widget::isUltimatelyBreadthFirst
		@param apiObject
		@param collectingBreadthFirst
			When true, this is a collecter and our parent (or parent's parent...) is the executor.
			When false, this is either depth first or executor, depending on the value of m_breadthFirst
		*/
		void addChildrenCommands( ApiEncapsulatedObjects &apiObject, bool collectingBreadthFirst );

	public:

		/// Returns true if this widget or any parent has m_breadthFirst set to true
		/// @see	Widget::m_breadthFirst
		/// @see	Widget::addChildrenCommands
		bool isUltimatelyBreadthFirst() const;

		/** Sets the new state, which affects skins.
			The state is is broadcasted to our children.
		@param state
			New state to transition to.
		@param smartHighlight
			When true and the Widget is already highlighted by a button, and now it's being
			highlighted by cursor (or viceversa), sets the new state to
			States::HighlightedButtonAndCursor
			If false, we just set exactly what was requested.
		@param broadcastEnable
			When false, if a child widget is States::Disabled; then we won't take it out of that state
			When true, we always extend this new state to our children.
		*/
		virtual void setState( States::States state, bool smartHighlight=true,
							   bool broadcastEnable=false );
		States::States getCurrentState() const;
		const WidgetVec& getChildren() const;
		/// Note it may be < getChildren().size(), as this value only returns pure widgets.
		/// Windows can be children from other windows, but are not counted for getNumWidgets
		/// See m_numWidgets
		size_t getNumWidgets() const;

		void setTransform( const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size,
						   const Ogre::Matrix3 &orientation );
		void setTransform( const Ogre::Vector2 &topLeft, const Ogre::Vector2 &size );
		void setTopLeft( const Ogre::Vector2 &topLeft );
		void setSize( const Ogre::Vector2 &size );
		void setOrientation( const Ogre::Matrix3 &orientation );

		void setCenter( const Ogre::Vector2 &center );
		Ogre::Vector2 getCenter() const;

		const Ogre::Vector2& getLocalTopLeft() const			{ return m_position; }
		Ogre::Vector2 getLocalBottomRight() const				{ return m_position + m_size; }
		const Ogre::Vector2& getSize() const					{ return m_size; }
		const Ogre::Matrix3& getOrientation() const				{ return m_orientation; }

		/** Establishes the clipping area to apply to our children widgets. Childrens
			will be clipped against:
			Legend:
			m_pos = m_position
			clipB = clipBorder

			m_pos-----------------------------------------------------------------------------
			|							m_pos.y+clipBTop									  |
			|					--------------------------------							  |
			|m_pos.x+clipBLeft |								|							  |
			|                  |								| m_pos.x+m_size.x-clipBRight |
			|					--------------------------------							  |
			|						m_pos.y+m_size.y-clipBBottom							  |
			--------------------------------------------------------------------m_pos + m_size

			Results are undefined if:
				clipBorders[Borders::Left] + clipBorders[Borders::Right] > m_size.x
				clipBorders[Borders::Top] + clipBorders[Borders::Bottom] > m_size.y
		@remarks
			These parameters call are likely going to be overwritten by
			Renderable::setClipBordersMatchSkin every time the virtual function Widget::setState
			is called
		@param clipBorders
		*/
		void setClipBorders( float clipBorders[colibrigui_nonnull Borders::NumBorders] );
		/// Returns m_position + clipBorderTopLeft; aka where the working area for children starts
		Ogre::Vector2 getTopLeftAfterClipping() const;
		/// Returns m_position + m_size - clipBorderBottomRight;
		/// aka where the working area for children ends
		Ogre::Vector2 getBottomRightAfterClipping() const;
		/// Returns the working area. clipBorderTopLeft + clipBorderBottomRight
		Ogre::Vector2 getSizeAfterClipping() const;

		const Ogre::Vector2& getBorderTopLeft() const		{ return m_clipBorderTL; }
		const Ogre::Vector2& getBorderBottomRight() const	{ return m_clipBorderBR; }

		/// Call this function before calling getDerivedTopLeft & co and it was asserting.
		/// Do not do it too often as it is not the most efficient solution.
		void updateDerivedTransformFromParent( bool updateParent=true );

		/// See ColibriManager::setTextEdit
		virtual void _setTextEdit( const char *text, int32_t selectStart, int32_t selectLength );

		/// See ColibriManager::setTextSpecialKeyPressed
		virtual void _setTextSpecialKey( uint32_t keyCode, uint16_t keyMod );

		/// See ColibriManager::setTextInput
		virtual void _setTextInput( const char *text );

		/// See ColibriManager::getImeLocation
		virtual Ogre::Vector2 _getImeLocation();

		/// See ColibriManager::isTextMultiline
		virtual bool isTextMultiline() const;

		/// See ColibriManager::focusedWantsTextInput
		virtual bool wantsTextInput() const;

		/// This function gets called every frame if the Widget
		/// registered itself for that.
		/// @see	ColibriManager::_addUpdateWidget
		virtual void _update( float timeSinceLast );

		virtual void _notifyCanvasChanged();

		/**
		@remarks
			Do not assume derived top left <= derived bottom right.
			If the widget is rotated by 180°, then bottom right will return a value
			at the top left corner, while the top left function will return a value
			at the bottom right corner.
		*/
		const Ogre::Vector2& getDerivedTopLeft() const;
		const Ogre::Vector2& getDerivedBottomRight() const;
		const Ogre::Matrix3& getDerivedOrientation() const;
		Ogre::Vector2 getDerivedCenter() const;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
