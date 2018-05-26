
#pragma once

#include "CrystalGui/CrystalRenderable.h"
#include "CrystalGui/Text/CrystalShaper.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	typedef std::vector<RichText> RichTextVec;

	class Label : public Renderable
	{
		struct Word
		{
			size_t offset;
			size_t length;
			Ogre::Vector2 startCaretPos;
			Ogre::Vector2 endCaretPos;
			Ogre::Vector2 lastAdvance;
			float lastCharWidth;
		};

		std::string		m_text[States::NumStates];
		RichTextVec		m_richText[States::NumStates];
		ShapedGlyphVec	m_shapes[States::NumStates];

		bool m_glyphsDirty[States::NumStates];
		bool m_glyphsPlaced[States::NumStates];
#if CRYSTALGUI_DEBUG_MEDIUM
		bool m_glyphsAligned[States::NumStates];
#endif
		/// For internal use. Set to true if any of RichText uses background, false otherwise.
		bool m_usesBackground;

		bool m_shadowOutline;
		Ogre::ColourValue m_shadowColour;
		Ogre::Vector2 m_shadowDisplace;

		Ogre::Vector2 m_backgroundSize;
		Ogre::ColourValue m_defaultBackgroundColour;

		LinebreakMode::LinebreakMode			m_linebreakMode;
		TextHorizAlignment::TextHorizAlignment	m_horizAlignment;
		TextVertAlignment::TextVertAlignment	m_vertAlignment;
		VertReadingDir::VertReadingDir			m_vertReadingDir;

		TextHorizAlignment::TextHorizAlignment	m_actualHorizAlignment[States::NumStates];
		VertReadingDir::VertReadingDir			m_actualVertReadingDir[States::NumStates];

		//Renderable	*m_background;

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
		void updateGlyphs( States::States state, bool bPlaceGlyphs=true );

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

		bool isAnyStateDirty() const;
		void flagDirty( States::States state );

		/// Returns false when there are no more words (and output is now empty)
		bool findNextWord( Word &inOutWord, States::States state ) const;
		float findLineMaxHeight( ShapedGlyphVec::const_iterator start,
								 States::States state ) const;

		inline void addQuad( GlyphVertex * RESTRICT_ALIAS vertexBuffer,
							 Ogre::Vector2 topLeft,
							 Ogre::Vector2 bottomRight,
							 uint16_t glyphWidth,
							 uint16_t glyphHeight,
							 uint32_t rgbaColour,
							 Ogre::Vector2 parentDerivedTL,
							 Ogre::Vector2 parentDerivedBR,
							 Ogre::Vector2 invSize,
							 uint32_t offset );

	public:
		Label( CrystalManager *manager );

		virtual bool isLabel() const		{ return true; }

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

		/** Called by CrystalManager after we've told them we're dirty.
			It will update m_shapes so we can correctly render text.
		@return
			True if the max number of glyphs has increased from the last time.
		*/
		bool _updateDirtyGlyphs();

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

		/** Recalculates the size of the widget based on the text contents to fit tightly.
			It may also reposition the Widget depending on newHorizPos & newVertPos
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
		void sizeToFit( States::States baseState,
						float maxAllowedWidth=std::numeric_limits<float>::max(),
						TextHorizAlignment::TextHorizAlignment newHorizPos=TextHorizAlignment::Left,
						TextVertAlignment::TextVertAlignment newVertPos=TextVertAlignment::Top );

		GlyphVertex* fillBackground( GlyphVertex * RESTRICT_ALIAS textVertBuffer,
									 const Ogre::Vector2 halfWindowRes,
									 const Ogre::Vector2 invWindowRes,
									 const Ogre::Vector2 parentDerivedTL,
									 const Ogre::Vector2 parentDerivedBR,
									 const bool isHorizontal );

		virtual void fillBuffersAndCommands( UiVertex * crystalgui_nonnull * crystalgui_nonnull
											 RESTRICT_ALIAS vertexBuffer,
											 GlyphVertex * crystalgui_nonnull * crystalgui_nonnull
											 RESTRICT_ALIAS textVertBuffer,
											 const Ogre::Vector2 &parentPos,
											 const Ogre::Vector2 &parentCurrentScrollPos,
											 const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
