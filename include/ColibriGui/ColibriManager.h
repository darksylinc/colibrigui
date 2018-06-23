
#pragma once

#include "ColibriGui/ColibriWidget.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	typedef std::vector<Label*> LabelVec;

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
		virtual void log( const char *text, Colibri::LogSeverity::LogSeverity severity ) {}
	};

	class ColibriManager
	{
	public:
		static const std::string c_defaultTextDatablockNames[States::NumStates];

	protected:
		WindowVec m_windows;
		LabelVec m_labels;
		/// Tracks total number of live widgets
		size_t m_numWidgets;
		size_t m_numLabels;
		size_t m_numTextGlyphs; /// It's an upper bound. Current max number of glyphs may be lower
		LabelVec m_dirtyLabels;
		WidgetVec m_dirtyWidgets;

		LogListener	*m_logListener;

		bool m_swapRTLControls;
		bool m_windowNavigationDirty;

		Ogre::Root					* colibrigui_nullable m_root;
		Ogre::VaoManager			* colibrigui_nullable m_vaoManager;
		Ogre::ObjectMemoryManager	* colibrigui_nullable m_objectMemoryManager;
		Ogre::SceneManager			* colibrigui_nullable m_sceneManager;
		Ogre::VertexArrayObject		* colibrigui_nullable m_vao;
		Ogre::VertexArrayObject		* colibrigui_nullable m_textVao;
		Ogre::IndirectBufferPacked	* colibrigui_nullable m_indirectBuffer;
		Ogre::CommandBuffer			* colibrigui_nullable m_commandBuffer;
		Ogre::HlmsDatablock			* colibrigui_nullable m_defaultTextDatablock[States::NumStates];

		Ogre::Vector2				m_canvasSize;
		Ogre::Vector2				m_invCanvasSize2x;
		Ogre::Vector2				m_pixelSize;
		Ogre::Vector2				m_pixelSize2x;
		Ogre::Vector2				m_halfWindowResolution;
		Ogre::Vector2				m_invWindowResolution2x;

		/// Window and/or Widget currently being in focus
		FocusPair		m_cursorFocusedPair;
		FocusPair		m_keyboardFocusedPair;
		bool			m_allowingScrollAlways;
		bool			m_allowingScrollGestureWhileButtonDown;
		bool			m_mouseCursorButtonDown;
		Ogre::Vector2	m_mouseCursorPosNdc; ///NDC = Normalized Device Coordinates
		bool			m_primaryButtonDown;
		Borders::Borders m_keyDirDown;
		float			m_keyRepeatWaitTimer;

		uint32_t		m_keyTextInputDown;

		/// Controls how much to wait before we start repeating
		float			m_keyRepeatDelay;
		/// Controls how fast we repeat
		float			m_timeDelayPerKeyStroke;

		uint32_t		m_defaultFontSize;

		SkinManager	*m_skinManager;
		ShaperManager *m_shaperManager;

		SkinInfo const * colibrigui_nullable
				m_defaultSkins[SkinWidgetTypes::NumSkinWidgetTypes][States::NumStates];

		void updateWidgetsFocusedByCursor();
		void updateAllDerivedTransforms();

		/// When pressing a mouse button on a widget, that overrides whatever keyboard was on.
		void overrideKeyboardFocusWith( const FocusPair &focusedPair );
		void overrideCursorFocusWith( const FocusPair &focusedPair );

		void updateDirtyLabels();
		void checkVertexBufferCapacity();

		template <typename T>
		void autosetNavigation( const std::vector<T> &container, size_t start, size_t numWidgets );

		void autosetNavigation( Window *window );

		/// Ensure its immediate parent window has the given widget within its visible bounds.
		void scrollToWidget( Widget *widget );

	public:
		ColibriManager( LogListener *logListener );
		~ColibriManager();

		void loadSkins( const char *fullPath );
		SkinManager* getSkinManager()								{ return m_skinManager; }

		ShaperManager* getShaperManager()							{ return m_shaperManager; }

		void setOgre( Ogre::Root * colibrigui_nullable root,
					  Ogre::VaoManager * colibrigui_nullable vaoManager,
					  Ogre::SceneManager * colibrigui_nullable sceneManager );
		Ogre::ObjectMemoryManager* getOgreObjectMemoryManager()		{ return m_objectMemoryManager; }
		Ogre::SceneManager* getOgreSceneManager()					{ return m_sceneManager; }
		Ogre::VertexArrayObject* getVao()							{ return m_vao; }
		Ogre::VertexArrayObject* getTextVao()						{ return m_textVao; }
		Ogre::HlmsDatablock * colibrigui_nonnull * colibrigui_nullable getDefaultTextDatablock()
																	{ return m_defaultTextDatablock; }

		/// When true, swaps the controls for RTL languages such as arabic. That means spinners
		/// increment when clicking left button, for example
		void setSwapRTLControls( bool swapRtl );
		bool swapRTLControls() const								{ return m_swapRTLControls; }
		bool shouldSwapRTL( HorizWidgetDir::HorizWidgetDir horizDir ) const
		{
			return horizDir == HorizWidgetDir::RTL ||
					(horizDir == HorizWidgetDir::AutoRTL && !m_swapRTLControls) ||
					(horizDir == HorizWidgetDir::AutoLTR && m_swapRTLControls);
		}

		uint32_t getDefaultFontSize26d6() const						{ return m_defaultFontSize; }

		/**	Sets the default skins to be used when creating a new widget.
			Usage:
			@code
				std::string skins[SkinWidgetTypes::NumSkinWidgetTypes];
				skins[SkinWidgetTypes::Button] = "MyDefaultButtonSkins";
				manager->setDefaultSkins( skins );
			@endcode
		@param defaultSkinPacks
			Array of skin pack names to use. It's the same as calling
			widget->setSkinPack( "MyDefaultButtonSkins" );
			Empty string means none (you'll have to assign the skin yourself)
		*/
		void setDefaultSkins( std::string defaultSkinPacks
							  [colibrigui_nonnull SkinWidgetTypes::NumSkinWidgetTypes] );
		SkinInfo const * colibrigui_nonnull const * colibrigui_nullable
				getDefaultSkin( SkinWidgetTypes::SkinWidgetTypes widgetType ) const;

		/// This pointers can be casted to HlmsColibriDatablock
		Ogre::HlmsDatablock * colibrigui_nullable getDefaultTextDatablock( States::States state ) const;

		const Ogre::Vector2& getMouseCursorPosNdc() const			{ return m_mouseCursorPosNdc; }

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
		@param windowResolution
			The actual size of the window. Does not necesarily have to match the virtual canvas
			size. We use this value to display sharp text.
		*/
		void setCanvasSize( const Ogre::Vector2 &canvasSize, const Ogre::Vector2 &pixelSize,
							const Ogre::Vector2 &windowResolution );
		const Ogre::Vector2& getCanvasSize() const					{ return m_canvasSize; }
		const Ogre::Vector2& getInvCanvasSize2x() const				{ return m_invCanvasSize2x; }
		const Ogre::Vector2& getPixelSize() const					{ return m_pixelSize; }
		const Ogre::Vector2& getPixelSize2x() const					{ return m_pixelSize2x; }
		const Ogre::Vector2& getHalfWindowResolution() const		{ return m_halfWindowResolution; }
		const Ogre::Vector2& getInvWindowResolution2x() const		{ return m_invWindowResolution2x; }

		void setMouseCursorMoved( Ogre::Vector2 newPosInCanvas );
		/**
		@param allowScrollGesture
			When true, moving the mouse cursor (or moving your finger, if touch) allows controlling
			scroll. This will cancel any hold/press of buttons.
			Typically you want this always true for touch interfaces, while for mouse cursors
			it depends on taste and how the interface was designed.
		@param alwaysAllowScroll
			When true, moving the mouse cursor always while holding the button (or your finger) will
			cancel the button being hold or hit; and will start scrolling instead even if the
			window in the foreground is not scrollable.
			When false, the action is only cancelled if the window is actually scrollable.
			The recommended value for touch screen is true, while false for mouse interfaces.
		*/
		void setMouseCursorPressed( bool allowScrollGesture, bool alwaysAllowScroll );
		void setMouseCursorReleased();
		void setKeyboardPrimaryPressed();
		void setKeyboardPrimaryReleased();
		void setCancel();
	protected:
		void updateKeyDirection( Borders::Borders direction );
	public:
		void setKeyDirectionPressed( Borders::Borders direction );
		void setKeyDirectionReleased( Borders::Borders direction );
		void setScroll( const Ogre::Vector2 &scrollAmount );

		/// For understanding these params, see SDL_TextEditingEvent
		void setTextEdit( const char *text, int32_t selectStart, int32_t selectLength );
		void setTextSpecialKeyPressed( uint32_t keyCode );
		void setTextSpecialKeyReleased( uint32_t keyCode );
		void setTextInput( const char *text );
		/// Returns true if the widget the keyboard is currently focused on accepts text input
		bool focusedWantsTextInput() const;

		void setLogListener( LogListener *logListener );
		LogListener* getLogListener() const		{ return m_logListener; }

		Window* createWindow( Window * colibrigui_nullable parent );

		/// Destroy the window and all of its children window and widgets
		void destroyWindow( Window *window );
		void destroyWidget( Widget *widget );

		/// For internal use. Do NOT call directly
		void _setAsParentlessWindow( Window *window );
		/// You can call this one directly
		void setAsParentlessWindow( Window *window );

		/// For internal use. Do NOT call directly
		void _scheduleSetTransformDirty( Widget *widget );

		/// Iterates through all windows and widgets, and calls setNextWidget to
		/// set which widgets is connected to each other (via an heuristic)
		void autosetNavigation();

		void _setWindowNavigationDirty();
		void _addDirtyLabel( Label *label );

		void update( float timeSinceLast );
		void prepareRenderCommands();
		void render();

#if __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
	protected:
		/// Parent cannot be null
		template <typename T>
		T * colibrigui_nonnull _createWidget( Widget * colibrigui_nonnull parent )
		{
			COLIBRI_ASSERT( parent && "parent must be provided!" );

			T *retVal = new T( this );

			retVal->_setParent( parent );
			retVal->_initialize();

			++m_numWidgets;

			return retVal;
		}
	public:
		/// Parent cannot be null
		template <typename T>
		T * colibrigui_nonnull createWidget( Widget * colibrigui_nonnull parent )
		{
			return _createWidget<T>( parent );
		}
#if __clang__
	#pragma clang diagnostic pop
#endif
	};

	template <>
	Label * colibrigui_nonnull ColibriManager::createWidget<Label>( Widget * colibrigui_nonnull parent );
}

COLIBRIGUI_ASSUME_NONNULL_END
