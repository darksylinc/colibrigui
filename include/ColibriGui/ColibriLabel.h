
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"
#include "ColibriGui/ColibriRenderable.h"
#include "ColibriGui/Text/ColibriShaper.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	typedef std::vector<RichText> RichTextVec;

	/** @ingroup Controls
	@class Label
	*/
	class Label : public Renderable
	{
	protected:
		struct Word
		{
			size_t offset;
			size_t length;
			Ogre::Vector2 startCaretPos;
			Ogre::Vector2 endCaretPos;
			Ogre::Vector2 lastAdvance;
			float lastCharWidth;
		};

		struct RasterHelper
		{
			LabelBmp                    *raster;
			std::map<uint32_t, uint32_t> glyphToRasterGlyphIdx;
		};

		std::string		m_text[States::NumStates];
		RichTextVec		m_richText[States::NumStates];
		ShapedGlyphVec	m_shapes[States::NumStates];

		bool m_glyphsDirty[States::NumStates];
		bool m_glyphsPlaced[States::NumStates];
#if COLIBRIGUI_DEBUG_MEDIUM
		bool m_glyphsAligned[States::NumStates];
#endif
		/// For internal use. Set to true if any of RichText uses background, false otherwise.
		bool m_usesBackground;

	public:
		/// When true (default) text will be clipped against the widget's size.
		///
		/// When false, the text may be drawn outside of the widget's rect
		/// (in either x and y direction) potentially overlapping with other
		/// widgets if you assume widget contents are restricted to their size
		/// and are tight together
		///
		/// PUBLIC VARIABLE. This variable can be altered directly.
		/// Changes are reflected immediately.
		bool m_clipTextToWidget;

	protected:
		bool m_shadowOutline;
		Ogre::ColourValue m_shadowColour;
		Ogre::Vector2 m_shadowDisplace;

		Ogre::Vector2 m_backgroundSize;
		Ogre::ColourValue m_defaultBackgroundColour;
		FontSize m_defaultFontSize;
		uint16_t m_defaultFont;

		LinebreakMode::LinebreakMode			m_linebreakMode;
		TextHorizAlignment::TextHorizAlignment	m_horizAlignment;
		TextVertAlignment::TextVertAlignment	m_vertAlignment;
		VertReadingDir::VertReadingDir			m_vertReadingDir;

		TextHorizAlignment::TextHorizAlignment	m_actualHorizAlignment[States::NumStates];
		VertReadingDir::VertReadingDir			m_actualVertReadingDir[States::NumStates];

		/// In case we have special symbols handled by a BMP font
		std::map<States::States, RasterHelper> m_rasterHelper;

		/** Checks RichText doesn't go out of bounds, and patches it if it does.
			If m_richText[state] is empty we'll create a default one for the whole string.
		@param state
		*/
		void validateRichText( States::States state );

		/** Checks if the string has changed. If so, requests the ShaperManager a new
			set of glyphs we can use

			If we already have calculated the glyphs for another state (i.e. States::States)
			we will reuse that data (unless the strings or its RichText are different)
		@param state
		@param bPlaceGlyphs
			When true, we will also call placeGlyphs
		*/
		colibri_virtual_l1 void updateGlyphs( States::States state, bool bPlaceGlyphs=true );

		/** Places the glyphs obtained from updateGlyphs at the correct position
			(always assuming TextHorizAlignment::Left) considering word wrap
			and size bounds.
		@param state
		@param performAlignment
			When true, we will also call alignGlyphs
		*/
		void placeGlyphs( States::States state, bool performAlignment=true );

		/** After calling placeGlyphs, this function realigns the text based on
			TextHorizAlignment & TextVertAlignment
		@param state
		@return
		*/
		void alignGlyphs( States::States state );
		void alignGlyphsHorizReadingDir( States::States state );
		void alignGlyphsVertReadingDir( States::States state );

	public:
		bool isAnyStateDirty() const;
	protected:
		void flagDirty( States::States state );

		/// Returns false when there are no more words (and output is now empty)
		bool findNextWord( Word &inOutWord, States::States state ) const;
		float findLineMaxHeight( ShapedGlyphVec::const_iterator start,
								 States::States state ) const;

		colibri_virtual_l1 inline void addQuad( GlyphVertex * RESTRICT_ALIAS vertexBuffer,
							 Ogre::Vector2 topLeft,
							 Ogre::Vector2 bottomRight,
							 uint16_t glyphWidth,
							 uint16_t glyphHeight,
							 uint32_t rgbaColour,
							 Ogre::Vector2 parentDerivedTL,
							 Ogre::Vector2 parentDerivedBR,
							 Ogre::Vector2 invSize,
							 uint32_t offset,
							 float canvasAspectRatio,
							 float invCanvasAspectRatio,
							 Matrix2x3 derivedRot );

	public:
		Label( ColibriManager *manager );

		void _destroy() colibri_override;

		bool isLabel() const colibri_override { return true; }

		/// Returns a RasterHelper for the given state. Creates one if it doesn't exist.
		RasterHelper *createRasterHelper( States::States state );

		/// Returns a RasterHelper for the given state. Nullptr if it doesn't exist.
		RasterHelper *colibrigui_nullable getRasterHelper( States::States state );

		/// Aligns the text horizontally relative to the widget's m_size
		/// Requires recalculating glyphs (i.e. same as setText)
		void setTextHorizAlignment( TextHorizAlignment::TextHorizAlignment horizAlignment );
		TextHorizAlignment::TextHorizAlignment getTextHorizAlignment() const;

		/// Aligns the text vertically relative to the widget's m_size
		/// Requires recalculating glyphs (i.e. same as setText)
		void setTextVertAlignment( TextVertAlignment::TextVertAlignment vertAlignment );
		TextVertAlignment::TextVertAlignment getTextVertAlignment() const;

		void setVertReadingDir( VertReadingDir::VertReadingDir vertReadingDir );
		VertReadingDir::VertReadingDir getVertReadingDir() const;

		/** Sets the default font size
		@remarks
			If the new default font size is different, and there are states (See States::States)
			that only have one rich edit entries (likely the one created by default, but not
			necessarily), they will be cleared
		*/
		void setDefaultFontSize( FontSize defaultFontSize );
		FontSize getDefaultFontSize() const							{ return m_defaultFontSize; }

		/** Sets the default font
		@remarks
			If the new default font is different, and there are states (See States::States)
			that only have one rich edit entries (likely the one created by default, but not
			necessarily), they will be cleared
		*/
		void setDefaultFont( uint16_t defaultFont );
		uint16_t getDefaultFont() const								{ return m_defaultFont; }

		/** Changes the colour of the current text (either all of it or a block); and sets
			the default colour for new text to the input colour
		@remarks
			This operations is very fast, as it doesn't need to reconstruct the glyphs
		@param colour
			The colour of the text
		@param richTextTextIdx
			The index of the rich text to modify. -1 to modify all blocks (i.e. the whole text)
			When out of bounds, it doesn't do anything except changing the default text colour
			for new text
		@param forState
			The state to modify. States::NumStates to modify all of them
		*/
		void setTextColour( const Ogre::ColourValue &colour, size_t richTextTextIdx = (size_t)-1,
							States::States forState = States::NumStates );

		/** Enables a shadow of the text behind each character, for highlighting or
			making the text easier to read, specially against backgrounds of the same
			colour as the text.
		@remarks
			This feature is controlled per Label, not per RichText entry.
			There is no overhead for calling this function often.
			Drawing the shadow may incur in higher GPU cost though, due to overdraw.
		@param enable
			True to enable. False to disable.
		@param shadowColour
			The colour of the shadow.
		@param shadowDisplace
			The direction to which to displace.
			Positive values displace towards bottom right.
			Value is in pixels.
		*/
		void setShadowOutline( bool enable, Ogre::ColourValue shadowColour=Ogre::ColourValue::Black,
							   const Ogre::Vector2 &shadowDisplace=Ogre::Vector2::UNIT_SCALE );

		/** Called by ColibriManager after we've told them we're dirty.
			It will update m_shapes so we can correctly render text.
		*/
		void _updateDirtyGlyphs();

		/** Returns the max number of glyphs needed to render
		@return
			It's not the sum of all states, but rather the maximum of all states,
			since only one state can be active at any given time.
		*/
		size_t getMaxNumGlyphs() const;

		/** Rich Edit settings are cleared when calling this function
		@param text
			Text must be UTF8
		@param forState
			Use NumStates to affect all states
		*/
		void setText( const std::string &text, States::States forState=States::NumStates );

		/// Returns the text for the given state. When state == States::NumStates, it
		/// returns the text from the current state
		const std::string& getText( States::States state=States::NumStates );


		/** Call this to modify rich text. MUST be called after Label::setText
		@remarks
			RichTextVec will be validated. Errors may be logged.
		@param richText
			When bSwap = false, richText is treated as const
			When bSwap = true, richText will be modified and its contents may be garbage
			after this call
		@param bSwap
			Use false if you care about preserving the contents of richText after this call

			When true, we will use a swap of the internal vector pointers
			to avoid a copy

			If state = States::NumStates, then we perform 1 swap, then copy to the other states
		@param forState
			Use NumStates to affect all states
		*/
		void setRichText( RichTextVec &richText, bool bSwap,
						  States::States forState = States::NumStates );

		/// Returns a newly constructed RichText filled with default values
		RichText getDefaultRichText() const;

		/** Returns the number of shapes/glyphs used by the current text from the selected state.
			Not to be confused with Label::getMaxNumGlyphs, which is used for rendering.
		@param state
			When value == States::NumStates, retrieves the current state
		@return
			The value in m_shapes[m_currentState].size()
		*/
		size_t getGlyphCount( States::States state=States::NumStates ) const;

		/** Retrieve the position of the caret (e.g. where to place a blinking cursor) based
			on the index to its glyph
		@remarks
			If glyphIdx is past the end of the last glyphs, the returned position will be to the
			right of the last glyph, rather than at the left.
		@param glyphIdx
			Index to the glyph. Out of bounds values get clamped.
		@param ptSize [out]
			Font size at that glyph
		@param outFontIdx [out]
			Font used for that glyph
		@return
			Top left to position the caret, in virtual canvas units.
		*/
		Ogre::Vector2 getCaretTopLeft( size_t glyphIdx, FontSize &ptSize, uint16_t &outFontIdx ) const;

		/// Multiple glyphs may be used to render the same cluster. Usually 1 glyph = 1 cluster,
		/// but if that's not the case, this function will tell you the index to the next glyph
		/// If input is out of bounds or output would go out of bounds,
		/// it returns m_shapes[m_currentState].size()
		size_t advanceGlyphToNextCluster( size_t glyphIdx ) const;

		/// @see	advanceGlyphToNextCluster
		/// If input is 0 it returns 0
		size_t regressGlyphToPreviousCluster( size_t glyphIdx ) const;

		/** Returns the start location in codeunits of a given glyph index
		@remarks
			If glyphIdx is out of bounds, then we return outLength = 0 and
			glyphStart = m_text[m_currentState].size() or 0 depending on whether
			RTL is swapped
		@param glyphIdx
		@param glyphStart [out]
			The start of the glyph in the string, in codeunits UTF16
		@param outLength [out]
			The length of glyph, in codeunits UTF16
		 */
		void getGlyphStartUtf16( size_t glyphIdx, size_t &glyphStart, size_t &outLength );

		/** Recalculates the size of the widget based on the text contents to fit tightly.
			It may also reposition the Widget depending on newHorizPos & newVertPos

			Q: Is it necessary to call this function?

			A: It's not necessary in the strict sense, but most likely you want to.
			The default size of most widgets is 0.
			Which means by default nothing will show up on screen!

			When you call Label::setText() we will calculate the vertices and render the atlas,
			texture both necessary to draw text on screen. However the size of the Label
			determines clipping. A size of 0 means the whole text will be clipped.
			You still need to call Widget::setSize() to fix that.

			By calling sizeToFit() you ensure we will call setSize() with a rect as tight as
			possible(*) that displays the whole text.

			Note that the rect set setSize() affects vertical and horizontal alignent.
			If you want right alignment but you call sizeToFit() on a single-line Label,
			then nothing will happen, because the text has no room to move left nor right

			(*)As tight as possible is a bit loose here. Font rendering is incredibly complex
			(thus innaccuracies and bugs appear) and the font in use may include arbitrary
			blank space because the designer so wanted it, thus you may notice the calculated
			AABB is not always 100% as tight as possible and could manually be shrunk further.
			However do not assume you can always shrink by an extra % because that vastly
			depends on the current string.

			Q: Why do I see samples not calling sizeToFit nor setSize yet
			text is displayed correctly?

			A: Because they rely on Layouts calling setSize automatically to fill the space.
			Most typically if the label does:

			@code
				// Set layout to cover this area. Cells inside will be modified to fit inside.
				// Note layout->setAdjustableWindow can be used instead, and also allows
				// for automatic scrolling if the window is not big enough to hold
				// all the elements
				layout->setCellSize( 1600, 900 );

				label->m_minSize = Ogre::Vector2( 0, 64 );
				label->m_expand[0] = true;
				label->m_proportion[0] = 1u;
				layout->m_vertical = true;
				layout->addCell( label );
				layout->layout();
			@endcode

			Then at least the width of the label will be automatically set by the layout engine,
			and the height is already set to a minimum of 64.
			However if a special font requires a very big glyph (e.g. arabic?) or the
			font size is enlarged, then that snippet is incorrect and the text will clip unless
			m_minSize.y is enlarged enough, or sizeToFit is called.

			Watch https://www.youtube.com/watch?v=4c9KD9-asaQ for better understanding
			of how layouts work and how layouts can be nested to achieve what you want.
			If video tutorials are too boring for you, adjust the playback speed to 1.5x or 2.0x

		@remarks
			TBD this function is slow as it requires synchronization with the rendering
			thread (not implemented yet hence TBD).
		@param baseState
			The state used to calculate the size
		@param maxAllowedWidth
			How large (in virtual canvas units) is the text allowed to grow.
			If a line of text requires more than maxAllowedWidth, it will be break into
			a newline.
		@param newHorizPos
			Specify the horizontal pivot point. If the value is other than
			TextHorizAlignment::Left, the widget will be repositioned.
		@param newVertPos
			Specify the vertical pivot point. If the value is other than
			TextHorizAlignment::Top, the widget will be repositioned.
		*/
		void sizeToFit( float maxAllowedWidth=std::numeric_limits<float>::max(),
						TextHorizAlignment::TextHorizAlignment newHorizPos=TextHorizAlignment::Left,
						TextVertAlignment::TextVertAlignment newVertPos=TextVertAlignment::Top,
						States::States baseState=States::NumStates );

		GlyphVertex* fillBackground( GlyphVertex * RESTRICT_ALIAS textVertBuffer,
									 const Ogre::Vector2 halfWindowRes,
									 const Ogre::Vector2 invWindowRes,
									 const Ogre::Vector2 parentDerivedTL,
									 const Ogre::Vector2 parentDerivedBR,
									 const bool isHorizontal );

		void _fillBuffersAndCommands(
			UiVertex *colibrigui_nonnull *colibrigui_nonnull RESTRICT_ALIAS vertexBuffer,
			GlyphVertex *colibrigui_nonnull *colibrigui_nonnull RESTRICT_ALIAS textVertBuffer,
			const Ogre::Vector2 &parentPos, const Ogre::Vector2 &parentCurrentScrollPos,
			const Matrix2x3 &parentRot ) colibri_override;

		void setTransformDirty( uint32_t dirtyReason ) colibri_final;

		void setState( States::States state, bool smartHighlight = true ) colibri_override;

		void _notifyCanvasChanged() colibri_override;
	};
}

COLIBRIGUI_ASSUME_NONNULL_END
