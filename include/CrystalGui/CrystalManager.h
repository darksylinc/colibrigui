
#pragma once

#include "CrystalGui/CrystalWidget.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	namespace LogSeverity
	{
		enum LogSeverity
		{
			/// Fatal error found and cannot continue, or severe error detected,
			/// attempting to continue could quite certainly cause corruption, crashes.
			Fatal,
			/// Severe error, but not as severe as fatal. However it's not
			/// impossible that corruption / crashes may happen.
			Error,
			/// Wrong usage detected; or a recoverable error.
			Warning,
			Info
		};
	}
	class LogListener
	{
	public:
		virtual void log( const char *text, LogSeverity::LogSeverity severity ) {}
	};

	class CrystalManager
	{
		WindowVec m_windows;

		LogListener	*m_logListener;

		bool m_windowNavigationDirty;
		bool m_childrenNavigationDirty;

		Ogre::VaoManager			*m_vaoManager;
		Ogre::ObjectMemoryManager	*m_objectMemoryManager;
		Ogre::SceneManager			*m_sceneManager;
		Ogre::IndexBufferPacked		*m_defaultIndexBuffer;

		template <typename T>
		void autosetNavigation( const std::vector<T> &container );

		void autosetNavigation( Window *window );

	public:
		CrystalManager();
		~CrystalManager();

		void setOgre( Ogre::VaoManager * crystalgui_nullable vaoManager );
		Ogre::ObjectMemoryManager* getOgreObjectMemoryManager()		{ return m_objectMemoryManager; }
		Ogre::SceneManager* getOgreSceneManager()					{ return m_sceneManager; }
		Ogre::IndexBufferPacked* getIndexBuffer()					{ return m_defaultIndexBuffer; }

		LogListener* getLogListener() const		{ return m_logListener; }

		Window* createWindow( Window * crystalgui_nullable parent );

		/// Destroy the window and all of its children window and widgets
		void destroyWindow( Window *window );

		/// For internal use. Do NOT call directly
		void _setAsParentlessWindow( Window *window );
		/// You can call this one directly
		void setAsParentlessWindow( Window *window );

		/// Iterates through all windows and widgets, and calls setNextWidget to
		/// set which widgets is connected to each other (via an heuristic)
		void autosetNavigation();

		void _setWindowNavigationDirty();
		void _notifyChildWindowIsDirty();

		void update();

#if __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
		/// Parent cannot be null
		template <typename T>
		T * crystalgui_nonnull createWidget( Widget * crystalgui_nonnull parent )
		{
			assert( parent && "parent must be provided!" );
			assert( !parent->isWindow() && "parent cannot be a window! Call createWindow instead" );

			T *retVal = new T( this );

			retVal->_setParent( parent );

			return retVal;
		}
#if __clang__
	#pragma clang diagnostic pop
#endif
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
