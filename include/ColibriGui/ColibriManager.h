
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgreIdString.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	typedef std::vector<Label*> LabelVec;
	typedef std::vector<LabelBmp*> LabelBmpVec;

	/**
	@class LogListener
	*/
	class LogListener
	{
	public:
		virtual ~LogListener();

		virtual void log( const char *text, Colibri::LogSeverity::LogSeverity severity ) {}
	};

	/**
	@class ColibriListener
	*/
	class ColibriListener
	{
	public:
		virtual ~ColibriListener();

		/** See SDL_SetClipboardText. We just send a null-terminated UTF8 string
		@param text
		*/
		virtual void setClipboardText( const char *text )		{}

		/** See SDL_GetClipboardText and SDL_HasClipboardText
		@param outText
			Pointer to text.
			outText [in] cannot be nullptr
			*outText [out] may be nullptr or valid ptr (if nullptr, then we return false)
		@return
			False if failed to grab the text or there was no text in the clipboard,
			true otherwise
		*/
		virtual bool getClipboardText( char * colibrigui_nonnull * const colibrigui_nullable outText )
		{
			*outText = 0;
			return false;
		}

		/** Frees/deletes the char* output by getClipboardText.
			We do the following guarantees:

				1. If getClipboardText returns false, freeClipboardText won't be called
				2. If getClipboardText never outputs a non-nullptr char*,
				   freeClipboardText will never receive a nullptr
		@param outText
		@return
		*/
		virtual void freeClipboardText( char *colibrigui_nullable text ) {}

		/**	When called, an Editbox was pressed and thus on Android and iOS a virtual
			keyboard or an input handler should be brought up.

			This is different from ColibriManager::focusedWantsTextInput because that's
			meant for IMEs that can stay on top all the time without interrupting user
			flow (i.e. seamless integration).

			This function instead is for Android & iOS that are best handled using an
			instrusive native UI element: they're brought up when the user selects the editbox,
			they're dismissed when the user hits Enter or taps outside

			These native UI elements are much better at handling the details (soft keyboards,
			copy/paste, passwords, Unicode, etc)

		@param editbox
			Editbox requesting the text input UI element
		*/
		virtual void showTextInput( Colibri::Editbox * /*editbox*/ ) {}
	};

	class ColibriManager
	{
		struct DelayedDestruction
		{
			Widget *widget;
			bool    windowVariantCalled;

			DelayedDestruction( Widget *_widget, bool _windowVariantCalled ) :
				widget( _widget ),
				windowVariantCalled( _windowVariantCalled )
			{
			}
		};

		typedef std::vector<DelayedDestruction> DelayedDestructionVec;

	public:
		static const std::string c_defaultTextDatablockNames[States::NumStates];

	protected:
		WindowVec m_windows;
		LabelVec m_labels;
		LabelBmpVec m_labelsBmp;
		/// Tracks total number of live widgets
		size_t m_numWidgets;
		size_t   m_numLabelsAndBmp;   /// Counts both Labels and LabelBmps
		size_t   m_numTextGlyphs;     /// It's an upper bound. Current max number of glyphs may be lower
		size_t   m_numTextGlyphsBmp;  /// It's an upper bound. Current max number of glyphs may be lower
		LabelVec m_dirtyLabels;
		LabelBmpVec m_dirtyLabelBmps;
		WidgetVec m_dirtyWidgets;
		/// Some widgets require getting called every frame for updates.
		/// Those widgets are listed here
		WidgetVec m_updateWidgets;
	public:
		/// When iterating in breadth first mode,
		///		m_breadthFirst[0] contains non Renderables in this iteration
		///		m_breadthFirst[1] contains Renderables in this iteration
		///		m_breadthFirst[2] contains non Renderables for the next iteration
		///		m_breadthFirst[3] contains Renderables for the next iteration
		/// After m_breadthFirst[0] and [1] are empty, we swap them with [2] and [3]
		///
		/// @remark	For internal use.
		/// @see	Widget::m_breadthFirst
		WidgetVec m_breadthFirst[4];

	protected:
		LogListener	*m_logListener;
		ColibriListener	*m_colibriListener;

		DelayedDestructionVec m_delayedDestruction;
		bool                  m_delayingDestruction;

		bool m_swapRTLControls;
		bool m_windowNavigationDirty;
		bool m_numGlyphsDirty;
		bool m_numGlyphsBmpDirty;

		bool m_widgetTransformsDirty;

		/// Is any widget dirty
		bool m_zOrderWidgetDirty;
		/// Is one of the windows stored by this manager immediately dirty.
		bool m_zOrderHasDirtyChildren;

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
		/// Size of a pixel on the screen in canvas units.
		Ogre::Vector2				m_pixelSize;
		Ogre::Vector2				m_pixelSize2x;
		Ogre::Vector2				m_halfWindowResolution;
		Ogre::Vector2				m_invWindowResolution2x;
		float						m_canvasAspectRatio;
		float						m_canvasInvAspectRatio;

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
		uint16_t		m_keyModInputDown;

		/// Controls how much to wait before we start repeating
		float			m_keyRepeatDelay;
		/// Controls how fast we repeat
		float			m_timeDelayPerKeyStroke;

		uint32_t		m_defaultFontSize;
	public:
		// These values are in virtual canvas units
		float			m_defaultTickmarkMargin;
		Ogre::Vector2	m_defaultTickmarkSize;
		float			m_defaultArrowMargin;
		Ogre::Vector2	m_defaultArrowSize;

	protected:
		SkinManager	*m_skinManager;
		ShaperManager *m_shaperManager;

		SkinInfo const * colibrigui_nullable
				m_defaultSkins[SkinWidgetTypes::NumSkinWidgetTypes][States::NumStates];
		Ogre::IdString m_defaultSkinPackNames[SkinWidgetTypes::NumSkinWidgetTypes];

		UiVertex		*m_vertexBufferBase;
		GlyphVertex		*m_textVertexBufferBase;

#if COLIBRIGUI_DEBUG_MEDIUM
		bool m_fillBuffersStarted;
		bool m_renderingStarted;
#endif

		static void reorderWindowVec( bool windowInListDirty, WindowVec& windows );

		void updateWidgetsFocusedByCursor();
		void updateAllDerivedTransforms();

		/// When pressing a mouse button on a widget, that overrides whatever keyboard was on.
		void overrideKeyboardFocusWith( const FocusPair &focusedPair );
		void overrideCursorFocusWith( const FocusPair &focusedPair );

	public:
		void _notifyNumGlyphsIsDirty();
		void _notifyNumGlyphsBmpIsDirty();
		void _updateDirtyLabels();

	protected:
		void checkVertexBufferCapacity();

		template <typename T>
		void autosetNavigation( const std::vector<T> &container, size_t start, size_t numWidgets );

		void autosetNavigation( Window *window );

		void updateZOrderDirty();

		/// Ensure its immediate parent window has the given widget within its visible bounds.
		void scrollToWidget( Widget *widget );

	public:
		ColibriManager( LogListener *logListener, ColibriListener *colibriListener );
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
		Ogre::HlmsManager *getOgreHlmsManager();

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
		GridLocations::GridLocations getSwappedGridLocation(
					GridLocations::GridLocations gridLoc ) const;

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

		Ogre::IdString getDefaultSkinPackName( SkinWidgetTypes::SkinWidgetTypes widgetType ) const;

		/// This pointers can be casted to HlmsColibriDatablock
		Ogre::HlmsDatablock * colibrigui_nullable getDefaultTextDatablock( States::States state ) const;

		const Ogre::Vector2& getMouseCursorPosNdc() const			{ return m_mouseCursorPosNdc; }

		/** Sets the size of the virtual canvas. All widgets are relative to this canvas
			For example if the canvas is 1920x1080, then a widget at x = 960 is in the
			middle of the canvas.

			This is irrespective of screen resolution. If the actual screen is 1024x768,
			the canvas will be stretched to fit the aspect ratio.

		@remarks
			This function can be slow as it must recalculate a lot of resolution-dependant
			variables for all widgets. Don't call this function unless something has changed.

		@param canvasSize
			The size of the canvas. A value of (1.0f, 1.0f) is screen independent with
			an aspect ratio of 1:1
		@param windowResolution
			The actual size of the window. Does not necesarily have to match the virtual canvas
			size. We use this value to display sharp text.
		*/
		void setCanvasSize( const Ogre::Vector2 &canvasSize, const Ogre::Vector2 &windowResolution );
		const Ogre::Vector2& getCanvasSize() const					{ return m_canvasSize; }
		const Ogre::Vector2& getInvCanvasSize2x() const				{ return m_invCanvasSize2x; }
		const Ogre::Vector2& getPixelSize() const					{ return m_pixelSize; }
		const Ogre::Vector2& getPixelSize2x() const					{ return m_pixelSize2x; }
		const Ogre::Vector2& getHalfWindowResolution() const		{ return m_halfWindowResolution; }
		const Ogre::Vector2& getInvWindowResolution2x() const		{ return m_invWindowResolution2x; }

		float getCanvasAspectRatio() const { return m_canvasAspectRatio; }
		float getCanvasInvAspectRatio() const { return m_canvasInvAspectRatio; }

		Ogre::Vector2 snapToPixels( const Ogre::Vector2 &canvasPos ) const;

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

		/// Returns true if the next call to setMouseCursorPressed/setMouseCursorReleased
		/// will be consumed by the widget currently focusing on a widget
		bool isMouseCursorFocusedOnWidget() const { return m_cursorFocusedPair.widget != 0; }

		void setKeyboardPrimaryPressed();
		void setKeyboardPrimaryReleased();
		void setCancel();

		void _notifyHighlightedWidgetDisabled( Widget *widget );
	protected:
		void updateKeyDirection( Borders::Borders direction );
	public:
		void setKeyDirectionPressed( Borders::Borders direction );
		void setKeyDirectionReleased( Borders::Borders direction );

		/// Returns the pair of window/widget that is currently
		/// being focused via keyboard navigation
		FocusPair getKeyboardFocusedPair() const { return m_keyboardFocusedPair; }

		/**
		@return
			True if the scroll was consumed by a widget.
			False otherwise
		*/
		bool setScroll( const Ogre::Vector2 &scrollAmount );

		/// For understanding these params, see SDL_TextEditingEvent
		void setTextEdit( const char *text, int32_t selectStart, int32_t selectLength );
		/**
		@see KeyCode::KeyCode
		@see KeyMod::KeyMod
		@param keyCode
		@param keyMod
			Bitmask from KeyMod::KeyMod
		*/
		void setTextSpecialKeyPressed( uint32_t keyCode, uint16_t keyMod );
		/// @see ColibriManager::setTextSpecialKeyPressed
		void setTextSpecialKeyReleased( uint32_t keyCode, uint16_t keyMod );

		/** Sets the text to the currently selected Editbox (if supports editing)
			This is meant for IMEs (input method editor) and Android/iOS keyboards
		@param text
			Input to text to add/replace
		@param bReplaceContents
			When false, the contents are appended
			When true, the contents replace current ones
		*/
		void setTextInput( const char *text, const bool bReplaceContents );
		/// Returns true if the widget the keyboard is currently focused on supports multiple
		/// lines. This means the app should not forward Enter presses as calls to
		/// setKeyboardPrimaryPressed/setKeyboardPrimaryReleased but rather as
		/// setTextSpecialKeyPressed/Released( '\\r' );
		bool isTextMultiline() const;
		/// Returns true if the widget the keyboard is currently focused on accepts text input
		bool focusedWantsTextInput() const;

		/// Returns the position where to locate the virtual keyboard / IME (for CJK languages)
		/// The value is in screen pixels
		Ogre::Vector2 getImeLocation();

		void setLogListener( LogListener *logListener );
		LogListener* getLogListener() const		{ return m_logListener; }

		void setColibriListener( ColibriListener *colibriListener );
		ColibriListener* getColibriListener() const		{ return m_colibriListener; }

		Window* createWindow( Window * colibrigui_nullable parent );

		/// Destroy the window and all of its children window and widgets
		void destroyWindow( Window *window );
		void destroyWidget( Widget *widget );

		bool _isDelayingDestruction() const { return m_delayingDestruction; }

		/// Safely calls widget->_callActionListeners( action )
		///
		/// By safely, it means that if a listener destroys 'widget',
		/// such destruction is delayed until it is safe to do so.
		void callActionListeners( Widget *widget, Action::Action action );

	protected:
		/// Actually execute destroyWindow/destroyWidget that has been delayed to avoid
		/// corrupting a widget while it was still in use (e.g. widget being destroyed
		/// as consequence of Widget::callActionListeners)
		void destroyDelayedWidgets();

	public:
		/// For internal use. Do NOT call directly
		void _setAsParentlessWindow( Window *window );
		/// You can call this one directly
		void setAsParentlessWindow( Window *window );

		/// Sometimes, Widgets cannot immediately call setTransformDirty, usually
		/// because they'll trigger asserts that either can be safely ignored,
		/// or they create side effects that cannot happen at that time.
		/// This function queues the widget so we call setTransformDirty
		/// for them later, inside ColibriManager::update
		/// For internal use. Do NOT call directly
		void _scheduleSetTransformDirty( Widget *widget );

		/// Some widgets require getting called every frame for updates.
		/// They register themselves via this interface.
		/// For internal use.
		void _addUpdateWidget( Widget *widget );
		void _removeUpdateWidget( Widget *widget );

		/// Iterates through all windows and widgets, and calls setNextWidget to
		/// set which widgets is connected to each other (via an heuristic)
		void autosetNavigation();

		void _setWindowNavigationDirty();

		void _setWidgetTransformsDirty();

		/// If creating a custom label widget, this must be called on creation.
		void _notifyLabelCreated( Label* label );

		/// If creating a custom label bmp widget, this must be called on creation.
		void _notifyLabelBmpCreated( LabelBmp* label );

		/** Notify the manager that a window has its z order dirty.
		@param windowInListDirty
			Should be true if a window this manager directly owns is dirty.
		*/
		void _setZOrderWindowDirty( bool windowInListDirty );
		void _addDirtyLabel( Label *label );
		void _addDirtyLabelBmp( LabelBmp *label );

		/// Cannot be nullptr
		void _stealKeyboardFocus( Widget *widget );

		void update( float timeSinceLast );
		void prepareRenderCommands();
		void render();

		const UiVertex* _getVertexBufferBase() const
		{
			COLIBRI_ASSERT_HIGH( m_fillBuffersStarted );
			return m_vertexBufferBase;
		}

		const GlyphVertex* _getTextVertexBufferBase() const
		{
			COLIBRI_ASSERT_HIGH( m_fillBuffersStarted );
			return m_textVertexBufferBase;
		}

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
	Label *colibrigui_nonnull ColibriManager::createWidget<Label>( Widget *colibrigui_nonnull parent );
	template <>
	LabelBmp *colibrigui_nonnull
			  ColibriManager::createWidget<LabelBmp>( Widget *colibrigui_nonnull parent );
}

COLIBRIGUI_ASSUME_NONNULL_END
