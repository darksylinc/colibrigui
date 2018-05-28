
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	/** Windows are special Widgets:
			1. They can only be children of other Windows
			2. m_parent can be nullptr
			3. Regular widgets can be navigated via left, up, down, etc.
			   Windows can only be navigated by entering or leaving them.
	*/
	class Window : public Renderable
	{
		friend class CrystalManager;

		Ogre::Vector2 m_currentScroll;
		/// For smooth scrolling, m_nextScroll contains the scroll destination,
		/// eventually over time m_currentScroll = m_nextScroll.
		Ogre::Vector2 m_nextScroll;
		Ogre::Vector2 m_maxScroll;

		/// The Window can be focused onto even if it has child windows.
		/// Does not work if a Window is currently modal.
		bool m_allowsFocusWithChildren;

		/// In range [0; m_windowsStart)
		uint16_t	m_defaultChildWidget;
		/// When true, all of our immediate children widgets are dirty and need to be recalculated
		bool		m_widgetNavigationDirty;
		/// When true, all of our immediate children windows are dirty and need to be recalculated
		bool		m_windowNavigationDirty;
		/// When true, all of our immediate children (widgets or windows)
		/// are not dirty, but one of our children's child is.
		bool		m_childrenNavigationDirty;

		WindowVec m_childWindows;

		void notifyChildWindowIsDirty();

		Window* getParentAsWindow() const;

		virtual size_t notifyParentChildIsDestroyed( Widget *childWidgetBeingRemoved );

	public:
		Window( CrystalManager *manager );
		virtual ~Window();

		virtual void _destroy();
		virtual bool isWindow() const	{ return true; }

		/** Smoothly scrolls from current location towards input destination.
		@param nextScroll
		@param animateOutOfRange
			When true, if the input is outside the range [0; m_maxScroll] this will be
			allowed and a small "bump" will be animated.
			When false, the input gets clamped and no bump is animated.
		*/
		void setScrollAnimated( const Ogre::Vector2 &nextScroll, bool animateOutOfRange );
		/// Immediately sets the scroll to the input destination.
		/// If the input is outside the range [0; m_maxScroll], it will be clamped.
		void setScrollImmediate( const Ogre::Vector2 &scroll );
		/// Return the current scroll position. Note that it may not be the final one,
		/// if it's still animating.
		/// To get the final scroll to achieve once animation finishes, use getNextScroll
		/// This value may be temporarily outside the range [0; m_maxScroll]
		virtual const Ogre::Vector2& getCurrentScroll() const;
		/// Returns the final scroll position. See getCurrentScroll
		/// This value may be temporarily outside the range [0; m_maxScroll]
		const Ogre::Vector2& getNextScroll() const						{ return m_nextScroll; }

		/// Sets the maximum scroll setScrollImmediate/setScrollAnimated can go. 0 to disable scrolling
		/// The value is in canvas space.
		void setMaxScroll( const Ogre::Vector2 &maxScroll );
		/// Calls setMaxScroll by calculating how much scroll is needed based on the current size,
		/// the clipping borders, and the input scrollable area you want the window to contain
		void calculateMaxScrollFromScrollableArea( const Ogre::Vector2 &scrollableArea );

		const Ogre::Vector2& getMaxScroll() const						{ return m_maxScroll; }
		Ogre::Vector2 getScrollableArea() const;

		/// Returns true if getMaxScroll() is non-zero in any direction.
		bool hasScroll() const;

		/// Calculates & sets the required scrollable area based on the current size of all child
		/// widgets & windows; and our current size.
		/// This function will not call sizeToFit on children. You'll likely want to call this last.
		void sizeScrollToFit();

		/// Returns true if it's still updating its scroll and the
		/// focused widget by the mouse cursor is potentially dirty
		bool update( float timeSinceLast );

		/// See Widget::setWidgetNavigationDirty
		/// Notifies all of our children widgets are dirty and we need to recalculate them.
		/// Also inform our parent windows they need to call us for recalculation
		virtual void setWidgetNavigationDirty();

		/// Similar to setWidgetNavigationDirty, but you should call this if this window
		/// has changed, and we'll inform our parent that it needs to recalculate
		/// all of its windows, and also broadcast to all parents they need to call our
		/// parent.
		void setWindowNavigationDirty();

		/// Reparents the given child window to this, 'this' becomes the parent
		void attachChild( Window *window );
		/// The given child window is detached; and becomes parentless (i.e. a main window)
		void detachChild( Window *window );
		/// Detaches from current parent. Does nothing if already parentless
		void detachFromParent();

		/// Makes this widget the default widget (i.e. which widget the cursor
		/// defaults to when the window is created)
		/// If widget is not our child or nullptr, the current default is unset
		void setDefault( Widget * crystalgui_nullable widget );
		Widget* crystalgui_nullable getDefaultWidget() const;

		FocusPair setIdleCursorMoved( const Ogre::Vector2 &newPosNdc );

		virtual void _updateDerivedTransformOnly( const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );

		virtual void _fillBuffersAndCommands( UiVertex * crystalgui_nonnull * crystalgui_nonnull
											  vertexBuffer,
											  GlyphVertex * crystalgui_nonnull * crystalgui_nonnull
											  textVertBuffer,
											  const Ogre::Vector2 &parentPos,
											  const Ogre::Vector2 &parentCurrentScrollPos,
											  const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
