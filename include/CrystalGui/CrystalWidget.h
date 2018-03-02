
#pragma once

#include "CrystalGui/CrystalGuiPrerequisites.h"

#include "OgreVector2.h"
#include "OgreMatrix3.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	struct UiVertex;
	typedef std::vector<Widget*> WidgetVec;
	typedef std::vector<Window*> WindowVec;

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
		friend class CrystalManager;
		friend class Renderable;

		/// Child and parent relationships are explicit, thus they're not tracked via
		/// WidgetListener::notifyWidgetDestroyed
		/// Can only be nullptr if 'this' is a Window.
		/// Windows can only be children of Windows.
		Widget * crystalgui_nonnull m_parent;
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

		CrystalManager          *m_manager;

		Widget * crystalgui_nullable	m_nextWidget[Borders::NumBorders];
		bool							m_autoSetNextWidget[Borders::NumBorders];
		bool					m_hidden;

		States::States			m_currentState;

		WidgetListenerPairVec	m_listeners;

		Ogre::Vector2	m_position;
		Ogre::Vector2	m_size;
		Ogre::Matrix3	m_orientation;

		Ogre::Vector2	m_derivedTopLeft;
		Ogre::Vector2	m_derivedBottomRight;
		Ogre::Matrix3	m_derivedOrientation;

#if CRYSTALGUI_DEBUG >= CRYSTALGUI_DEBUG_MEDIUM
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

		void setTransformDirty();

	public:
		Widget( CrystalManager *manager );
		virtual ~Widget();

		virtual void _destroy();

		/// Do not call directly. 'this' cannot be a Window
		void _setParent( Widget *parent );

		virtual bool isRenderable() const	{ return false; }
		virtual bool isWindow() const		{ return false; }

		/// If 'this' is a window, it returns 'this'. Otherwise it returns its
		/// parent (or its parent's parent) until we find a window. Cannot
		/// return null
		Window* getFirstParentWindow();

		// WidgetListener overload
		virtual void notifyWidgetDestroyed( Widget *widget );

		void addListener( WidgetListener *listener );
		void removeListener( WidgetListener *listener );

		/** Call this to indicate the widget has been moved or created; thus we'll broadcast
			the message until reaching our most immediate parent window.
			This window is now aware we've changed, and will later recalculate all
			the connections of its children.
			See setNextWidget
		*/
		virtual void setWidgetNavigationDirty();

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
		void setNextWidget( Widget * crystalgui_nullable nextWidget,
							Borders::Borders direction, bool reciprocate=true );

		float getRight() const;
		float getBottom() const;

		/**
		@return
			True if the given widget is partially intersecting or fully inside this, or viceversa.
		*/
		bool intersectsChild( Widget *child ) const;
		//bool intersects( Widget *widget ) const;

		/// Input must be in derived coordinates i.e. the canvas in range [-1; 1]
		bool intersects( const Ogre::Vector2 &pos ) const;

		virtual void broadcastNewVao( Ogre::VertexArrayObject *vao );

		virtual UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );

		void setState( States::States state );
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

		const Ogre::Vector2& getPosition() const				{ return m_position; }
		const Ogre::Vector2& getSize() const					{ return m_size; }
		const Ogre::Matrix3& getOrientation() const				{ return m_orientation; }

		/// Call this function before calling getDerivedTopLeft & co and it was asserting.
		/// Do not do it too often as it is not the most efficient solution.
		void updateDerivedTransformFromParent();

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

CRYSTALGUI_ASSUME_NONNULL_END
