
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
			/// This is for very bad things, like double frees detected, dangling pointers,
			/// divisions by zero, integer/buffer overflow, inconsistent state, etc.
			Fatal,
			/// Severe error, but not as severe as fatal. However it's not
			/// impossible that corruption / crashes could happen.
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
		/// Tracks total number of live widgets
		size_t m_numWidgets;

		LogListener	*m_logListener;

		bool m_windowNavigationDirty;
		bool m_childrenNavigationDirty;

		Ogre::Root					*m_root;
		Ogre::VaoManager			*m_vaoManager;
		Ogre::ObjectMemoryManager	*m_objectMemoryManager;
		Ogre::SceneManager			*m_sceneManager;
		Ogre::VertexArrayObject		*m_vao;
		Ogre::IndirectBufferPacked	*m_indirectBuffer;
		Ogre::CommandBuffer			*m_commandBuffer;

		Ogre::Vector2				m_canvasSize;
		Ogre::Vector2				m_invCanvasSize2x;

		SkinManager	*m_skinManager;

		void checkVertexBufferCapacity();

		template <typename T>
		void autosetNavigation( const std::vector<T> &container );

		void autosetNavigation( Window *window );

	public:
		CrystalManager();
		~CrystalManager();

		void loadSkins( const char *fullPath );
		SkinManager* getSkinManager()								{ return m_skinManager; }

		void setOgre( Ogre::Root * crystalgui_nullable root,
					  Ogre::VaoManager * crystalgui_nullable vaoManager,
					  Ogre::SceneManager * crystalgui_nullable sceneManager );
		Ogre::ObjectMemoryManager* getOgreObjectMemoryManager()		{ return m_objectMemoryManager; }
		Ogre::SceneManager* getOgreSceneManager()					{ return m_sceneManager; }
		Ogre::VertexArrayObject* getVao()							{ return m_vao; }

		void setCanvasSize( const Ogre::Vector2 &canvasSize );
		const Ogre::Vector2& getCanvasSize() const					{ return m_canvasSize; }
		const Ogre::Vector2& getInvCanvasSize2x() const				{ return m_invCanvasSize2x; }

		LogListener* getLogListener() const		{ return m_logListener; }

		Window* createWindow( Window * crystalgui_nullable parent );

		/// Destroy the window and all of its children window and widgets
		void destroyWindow( Window *window );
		void destroyWidget( Widget *widget );

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
		void prepareRenderCommands();
		void render();

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

			++m_numWidgets;

			return retVal;
		}
#if __clang__
	#pragma clang diagnostic pop
#endif
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
