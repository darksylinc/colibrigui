
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

		/// Child and parent relationships are explicit, thus they're not tracked via
		/// WidgetListener::notifyWidgetDestroyed
		/// Can only be nullptr if 'this' is a Window.
		Widget * crystalgui_nonnull m_parent;
		WidgetVec					m_children;

		CrystalManager          *m_manager;

		Widget * crystalgui_nullable	m_nextWidget[Borders::NumBorders];
		bool							m_autoSetNextWidget[Borders::NumBorders];
		bool					m_hidden;

		States::States			m_currentState;

		WidgetListenerPairVec	m_listeners;

		WidgetListenerPairVec::iterator findListener( WidgetListener *listener );

		static Ogre::Vector2 mul( const Ogre::Matrix3 &matrix, Ogre::Vector2 xyPos );

		void updateDerivedTransform( const Ogre::Vector2 &parentPos, const Ogre::Matrix3 &parentRot );

		/// Asumes:
		///		1. Parent is already up to date
		///		2. m_parent is non-null (i.e. Window breaks this assumption)
		void updateDerivedTransformFromParent();

	public:
		Ogre::Vector2   m_position;
		Ogre::Vector2   m_size;
		Ogre::Matrix3   m_orientation;

		Ogre::Vector2   m_derivedTopLeft;
		Ogre::Vector2   m_derivedBottomRight;
		Ogre::Matrix3   m_derivedOrientation;

		Widget( CrystalManager *manager );
		virtual ~Widget();

		virtual void _destroy();

		virtual bool isWindow() const	{ return false; }

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
		void setNextWidget( Widget *nextWidget, Borders::Borders direction, bool reciprocate=true );

		float getRight() const;
		float getBottom() const;

		/**
		@return
			True if the given widget is partially intersecting or fully inside this, or viceversa.
		*/
		bool intersects( Widget *widget ) const;

		virtual UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );

		States::States getCurrentState() const;
		const WidgetVec& getChildren() const;
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
