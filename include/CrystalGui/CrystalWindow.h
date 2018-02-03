
#pragma once

#include "CrystalGui/CrystalRenderable.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	class Window : public Renderable
	{
		friend class CrystalManager;

		/// In range [0; m_children.size())
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

	public:
		Window( CrystalManager *manager );
		virtual ~Window();

		virtual void _destroy();
		virtual bool isWindow() const	{ return true; }

		virtual void notifyWidgetDestroyed( Widget *widget );

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
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
