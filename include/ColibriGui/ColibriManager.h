
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgreIdString.h"

COLIBRI_ASSUME_NONNULL_BEGIN

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
		virtual bool getClipboardText( char * colibri_nonnull * const colibri_nullable outText )
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
		virtual void freeClipboardText( char *colibri_nullable text ) {}

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

		/** Notifies when canvas has changed (i.e. ColibriManager::setCanvasSize was called)
		@remarks
			This function gets called a little late, before rendering.
			If you create or destroy Widgets or alter a Label's content, you must
			call ColibriManager::update
		*/
		virtual void notifyCanvasOrResolutionUpdated() {}

		/** Called when derived class should "react".

			By reaction we're often talking about playing a sound effect, but not necessarily.

			@note The user can call ColibriManager::setEffectReaction while inside
			WidgetActionListener::notifyWidgetAction to override the default value set by ColibriManager.
			This is useful if e.g. the widget itself was hit but conditions to proceed weren't met and a
			different SFX should be played (like an error SFX, close SFX, or no SFX).
			Or perhaps the widget is special (e.g. a volume slider is different from normal sliders).

			@note Colibri does NOT use this value in any way. Hence you can set arbitrary values via
			setEffectReaction(), but do not expect the return value to stick as it may change with
			the next setKeyDirectionReleased(), etc.<br/>
			It is the user who gives real meaning to this value.

		@param effectReaction
			See EffectReaction::EffectReaction.
			The value may be outside range if you set it to something else via
			ColibriManager::setEffectReaction.
		@param repeatCount
			The number of times the same effect has accumulated (in less than one frame) because movement
			and key input may repeat multiple times per second.
			Bbeware if repeatCount > 1, do not play two identical SFX at the same time, you *should* add
			some artificial delay.
		*/
		virtual void flushEffectReaction( uint16_t /*effectReaction*/, uint16_t /*repeatCount*/ ) {}
	};

	namespace EffectReaction
	{
		enum EffectReaction
		{
			/// Caller must do nothing.
			NoReaction,
			/// Caller should play the "highlight changed" sound effect.
			Moved,
			/// Caller should play the "PrimaryAction" sound effect.
			PrimaryAction,
			/// Caller wanted to play the "PrimaryAction" but the button was not pressable.
			NotPressable,
			/// Text was successfully inserted.
			TextInputInserted,
			/// Text was successfully removed.
			TextInputRemoved,
			/// Text was supposed to be removed, but can't (e.g. hit Backspace at the beginning).
			TextInputRemoveFailed,
			/// We got stuck in some non-renderable UTF character and had to clear the Editbox.
			TextInputErrorClear,
			/// The cursor/caret was moved but no actual text was added or removed.
			TextInputCaretNavigation,
			/// Slider was attempted to change (may have been clamped, or set to
			/// a wrong direction e.g. pressed "Up" instead of left/right).
			SliderMoved,
			/// Spinner was attempted to change (may have been clamped, or set to
			/// a wrong direction e.g. pressed "Up" instead of left/right).
			SpinnerChanged,
			/// ToggleButton was toggled.
			Toggled,
			/// Checkbox changed.
			Checkboxed,
		};
	}

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
		size_t   m_numCustomShapesVertices;
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

		bool m_touchOnlyMode;

		const bool m_multipass;

		Ogre::Root *colibri_nullable                m_root;
		Ogre::VaoManager *colibri_nullable          m_vaoManager;
		Ogre::ObjectMemoryManager *colibri_nullable m_objectMemoryManager;
		Ogre::SceneManager *colibri_nullable        m_sceneManager;
		Ogre::VertexArrayObject *colibri_nullable   m_vao;
		Ogre::VertexArrayObject *colibri_nullable   m_textVao;
		std::vector<Ogre::IndirectBufferPacked *>   m_indirectBuffer;
		uint32_t                                    m_currIndirectBuffer;
		Ogre::CommandBuffer *colibri_nullable       m_commandBuffer;
		Ogre::HlmsDatablock *colibri_nullable       m_defaultTextDatablock[States::NumStates];

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
		Ogre::Vector2	m_scrollHappened;
		Ogre::Vector2	m_mouseCursorPosNdc; ///NDC = Normalized Device Coordinates
		bool			m_primaryButtonDown;
		Borders::Borders m_keyDirDown;
		float			m_keyRepeatWaitTimer;

		/// See EffectReaction::EffectReaction. Note the value could be outside range.
		uint16_t m_effectReaction;
		uint16_t m_effectReactionRepeatCount;

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

		SkinInfo const * colibri_nullable
				m_defaultSkins[SkinWidgetTypes::NumSkinWidgetTypes][States::NumStates];
		Ogre::IdString m_defaultSkinPackNames[SkinWidgetTypes::NumSkinWidgetTypes];

		UiVertex    *m_vertexBufferBase;
		GlyphVertex *m_textVertexBufferBase;

		/// Only used when m_multipass == true
		std::vector<uint8_t> m_multipassTmpBuffer;

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
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

		UiVertex    *getMultipassVertexBuffer( size_t numElements, size_t textNumElements );
		GlyphVertex *getMultipassTextVertexBuffer( size_t numElements, size_t textNumElements );

		template <typename T>
		void autosetNavigation( const std::vector<T> &container, size_t start, size_t numWidgets );

		void autosetNavigation( Window *window );

		void updateZOrderDirty();

		/// Ensure its immediate parent window has the given widget within its visible bounds.
		void scrollToWidget( Widget *widget );

	public:
		/**
		@param logListener
		@param colibriListener
		@param multipass
			Set this to true to allow rendering more than once in the same frame (otherwise you'll
			get exceptions about mapping the same buffer twice in the same frame).

			Recommended value for the main UI is false if you don't need it; since multipass is not
			mobile-friendly and *might* be slower on Desktop too.
		@param bSecondary
			If this value is set to true, you must call _setPrimary.
			See OffScreenCanvas's implementation.
		*/
		ColibriManager( LogListener *logListener, ColibriListener *colibriListener,
						bool multipass = false, bool bSecondary = false );
		~ColibriManager();

		void loadSkins( const char *fullPath );
		SkinManager* getSkinManager()								{ return m_skinManager; }

		ShaperManager* getShaperManager()							{ return m_shaperManager; }

		/** This function allows to create secondary managers (i.e. for offscreen rendering)
			while sharing resources.

			@note ColibriManager must've been constructed with bSecondary = true. See ColibriManager().
			@note The primary is assumed to have been fully initialized, including its skins and fonts.
		@param primaryManager
			The main manager who owns 'this' secondary ColibriManager.
		*/
		void _setPrimary( ColibriManager *primaryManager );

		/// Returns whether this manager is the primary or not.
		bool isPrimary() const;

		bool isMultipass() const { return m_multipass; }

		void setOgre( Ogre::Root *colibri_nullable root, Ogre::VaoManager *colibri_nullable vaoManager,
					  Ogre::SceneManager *colibri_nullable sceneManager );
		Ogre::ObjectMemoryManager* getOgreObjectMemoryManager()		{ return m_objectMemoryManager; }
		Ogre::Root                *getOgreRoot() { return m_root; }
		Ogre::SceneManager* getOgreSceneManager()					{ return m_sceneManager; }
		Ogre::VertexArrayObject* getVao()							{ return m_vao; }
		Ogre::VertexArrayObject* getTextVao()						{ return m_textVao; }
		Ogre::HlmsDatablock * colibri_nonnull * colibri_nullable getDefaultTextDatablock()
																	{ return m_defaultTextDatablock; }
		Ogre::HlmsManager *getOgreHlmsManager();

		Ogre::IndirectBufferPacked *getIndirectBuffer();

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

		/** Sets a new default font size.
			This takes effect for new Labels created and Labels that are
			modified in a way they end up requerying ColibriManager::getDefaultFontSize26d6

			Therefore it's advised to call this function as early as possible,
			or recreate the entire UI when changed (i.e. typically in Options)
		@param defaultFontSize
			New default font size. Value is in 26d6 format i.e. use FontSize( 18.0f ).value26d6
		*/
		void setDefaultFontSize26d6( uint32_t defaultFontSize ) { m_defaultFontSize = defaultFontSize; }

		uint32_t getDefaultFontSize26d6() const						{ return m_defaultFontSize; }

		/** Devices that have no keyboard and no gamepad don't need as
			many states as there are in States.

			Setting this value to true will cause newly created Widgets (and further calls to
			Renderable::setSkinPack & Renderable::_setSkinPack) to replace skins for
			HighlightedButtonAndCursor with HighlightedButton and
			HighlightedCursor skin with Idle.

			Thus internally these events still happen, but visually the widgets will look
			like it does by their replaced versions.

			This means there's fewer possible "visual" states, causing less confusion.
		@param bTouchOnlyMode
			True to set touch only mode (i.e. preferred method on iOS & Android unless a
			gamepad/keyboard is attached and you support them).
			False to disable touch only mode.
		*/
		void setTouchOnlyMode( bool bTouchOnlyMode );
		bool getTouchOnlyMode() const { return m_touchOnlyMode; }

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
							  [colibri_nonnull SkinWidgetTypes::NumSkinWidgetTypes] );
		SkinInfo const * colibri_nullable const * colibri_nonnull
				getDefaultSkin( SkinWidgetTypes::SkinWidgetTypes widgetType ) const;

		Ogre::IdString getDefaultSkinPackName( SkinWidgetTypes::SkinWidgetTypes widgetType ) const;

		/// This pointers can be casted to HlmsColibriDatablock
		Ogre::HlmsDatablock * colibri_nullable getDefaultTextDatablock( States::States state ) const;

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
		/// being focused via the cursor
		FocusPair getCursorFocusedPair() const { return m_cursorFocusedPair; }

		/// Returns the pair of window/widget that is currently
		/// being focused via keyboard navigation
		FocusPair getKeyboardFocusedPair() const { return m_keyboardFocusedPair; }

		/** Sets what SFX should be play next, unless overriden before the next call to
			flushEffectReaction().
		@param effectReaction
			See EffectReaction::EffectReaction. The value can be outside range.
		@param repeatCount
		*/
		void setEffectReaction( uint16_t effectReaction, uint16_t repeatCount = 1u )
		{
			if( m_effectReaction == effectReaction )
				m_effectReactionRepeatCount += repeatCount;
			else
			{
				m_effectReaction = effectReaction;
				m_effectReactionRepeatCount = repeatCount;
			}
		}

		/**  See setEffectReaction() and see ColibriListener::flushEffectReaction.
		@return
			See EffectReaction::EffectReaction. The value may be outside range.
		*/
		uint16_t getEffectReaction() const { return m_effectReaction; }

		/// See ColibriListener::flushEffectReaction.
		uint16_t getEffectReactionRepeatCount() const { return m_effectReactionRepeatCount; }

		/// Calls ColibriListener::flushEffectReaction.
		void flushEffectReaction();

		/**
		@param animated
			If true the scroll will be animated.
		@return
			True if the scroll was consumed by a widget.
			False otherwise
		*/
		bool setScroll( const Ogre::Vector2 &scrollAmount, bool animated = true );

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
		@remarks
			WARNING: Call this function before ColibriManager::update.
			If you call this between ColibriManager::update and ColibriManager::prepareRenderCommands
			asserts could trigger and/or crashes happen
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

		Window* createWindow( Window * colibri_nullable parent );

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

		/** Notify the manager that a CustomShape has changed its vertex count
		@param vertexCountDiff
			Positive with the number of new vertices.
			Negative with the number of removed vertices.
		*/
		void _addCustomShapesVertexCountChange( int32_t vertexCountDiff );

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
		T * colibri_nonnull _createWidget( Widget * colibri_nonnull parent )
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
		T * colibri_nonnull createWidget( Widget * colibri_nonnull parent )
		{
			return _createWidget<T>( parent );
		}
#if __clang__
	#pragma clang diagnostic pop
#endif
	};

	template <>
	Label *colibri_nonnull ColibriManager::createWidget<Label>( Widget *colibri_nonnull parent );
	template <>
	LabelBmp *colibri_nonnull
			  ColibriManager::createWidget<LabelBmp>( Widget *colibri_nonnull parent );
}

COLIBRI_ASSUME_NONNULL_END
