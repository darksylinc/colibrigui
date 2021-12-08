
#pragma once

#include "ColibriGui/ColibriGuiPrerequisites.h"

#include "ColibriGui/ColibriRenderable.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	class BmpFont;
	struct BmpGlyph;
	typedef std::vector<BmpGlyph> BmpGlyphVec;

	/** @ingroup Controls
	@class LabelBmp
		An extremely simplified version of Label that renders using BMP fonts
		instead of relying on FreeType fonts.

		We specifically support a subset of AngelCode BMFonts:
		https://www.angelcode.com/products/bmfont/doc/file_format.html

		This is only meant for displaying text that was designed to be rendered
		using a specific monospace BMP font (i.e. all characters have same
		width and height).

		It is also useful for very fast ASCII monospace text rendering
		(e.g. debug data)

		Its Unicode support is basic or non-existent.

		This widget has very specific use-cases.
		Refer to Label for normal text rendering.
	*/
	class LabelBmp : public Renderable
	{
	protected:
		std::string m_text[States::NumStates];

		BmpGlyphVec m_shapes;

		bool m_glyphsDirty;

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
		bool              m_shadowOutline;
		Ogre::ColourValue m_shadowColour;
		Ogre::Vector2     m_shadowDisplace;

		FontSize m_fontSize;
		uint16_t m_font;

		/** Checks if the string has changed. If so, requests the ShaperManager a new
			set of glyphs we can use

			If we already have calculated the glyphs for another state (i.e. States::States)
			we will reuse that data (unless the strings or its RichText are different)
		@param state
		@param bPlaceGlyphs
			When true, we will also call placeGlyphs
		*/
		colibri_virtual_l1 void updateGlyphs();

	public:
		bool isLabelBmpDirty() const;

	protected:
		void flagDirty();

	public:
		LabelBmp( ColibriManager *manager );

		bool isLabelBmp() const colibri_override { return true; }

		/// Sets the font size
		void     setFontSize( FontSize defaultFontSize );
		FontSize getFontSize() const { return m_fontSize; }

		/// Sets the font
		void     setFont( uint16_t font );
		uint16_t getFont() const { return m_font; }

		/** Changes the colour of the current text; and sets
			the default colour for new text to the input colour
		@remarks
			This operations is very fast, as it doesn't need to reconstruct the glyphs
		@param colour
			The colour of the text
		*/
		void setTextColour( const Ogre::ColourValue &colour );

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
		void setShadowOutline( bool enable, Ogre::ColourValue shadowColour = Ogre::ColourValue::Black,
							   const Ogre::Vector2 &shadowDisplace = Ogre::Vector2::UNIT_SCALE );

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
		void setText( const std::string &text, States::States forState = States::NumStates );

		/// Returns the text for the given state. When state == States::NumStates, it
		/// returns the text from the current state
		const std::string &getText( States::States state = States::NumStates );

		/// Recalculates the size of the widget based on the text contents to fit tightly.
		void sizeToFit();

		void _fillBuffersAndCommands(
			UiVertex *colibrigui_nonnull *colibrigui_nonnull RESTRICT_ALIAS vertexBuffer,
			GlyphVertex *colibrigui_nonnull *colibrigui_nonnull RESTRICT_ALIAS textVertBuffer,
			const Ogre::Vector2 &parentPos, const Ogre::Vector2 &parentCurrentScrollPos,
			const Matrix2x3 &parentRot ) colibri_override;

		void setState( States::States state, bool smartHighlight = true ) colibri_override;
	};
}  // namespace Colibri

COLIBRIGUI_ASSUME_NONNULL_END
