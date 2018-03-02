
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
		Ogre::Vector2				m_pixelSize;

		/// Window and/or Widget currently being in focus
		FocusPair	m_focusedPair;

		SkinManager	*m_skinManager;

		void checkVertexBufferCapacity();

		template <typename T>
		void autosetNavigation( const std::vector<T> &container, size_t start, size_t numWidgets );

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

		/** Sets the size of the virtual canvas. All widgets are relative to this canvas
			For example if the canvas is 1920x1080, then a widget at x = 960 is in the
			middle of the canvas.

			This is irrespective of screen resolution. If the actual screen is 1024x768,
			the canvas will be stretched to fit the aspect ratio.
		@param canvasSize
			The size of the canvas. A value of (1.0f, 1.0f) is screen independent with
			an aspect ratio of 1:1
		@param pixelSize
			How many units a pixel occupies. This value is used to determine the size of the
			widget borders.
			Usually this value should be 1 (e.g. canvas = 1920x1080, pixelSize = 1). However
			if you insert a canvas of 1x1; then your pixel size could be (1 / 1920, 1 / 1080)
		*/
		void setCanvasSize( const Ogre::Vector2 &canvasSize, const Ogre::Vector2 &pixelSize );
		const Ogre::Vector2& getCanvasSize() const					{ return m_canvasSize; }
		const Ogre::Vector2& getInvCanvasSize2x() const				{ return m_invCanvasSize2x; }
		const Ogre::Vector2& getPixelSize() const					{ return m_pixelSize; }

		void setCursorMoved( Ogre::Vector2 newPosInCanvas );

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
			CRYSTAL_ASSERT( parent && "parent must be provided!" );

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
