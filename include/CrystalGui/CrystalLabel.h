
#pragma once

#include "CrystalGui/CrystalRenderable.h"
#include "CrystalGui/Text/CrystalShaper.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	typedef std::vector<RichText> RichTextVec;

	class Label : public Renderable
	{
		std::string		m_text[States::NumStates];
		RichTextVec		m_richText[States::NumStates];
		ShapedGlyphVec	m_shapes[States::NumStates];

		bool m_glyphsDirty[States::NumStates];

		LinebreakMode::LinebreakMode		m_linebreakMode;
		HorizReadingDir::HorizReadingDir	m_horizReadingDir;
		VertReadingDir::VertReadingDir		m_vertReadingDir;

		//Renderable	*m_background;

		void validateRichText( States::States state );
		void updateGlyphs( States::States state );

		bool isAnyStateDirty() const;
		void flagDirty( States::States state );

		uint16_t findLineMaxHeight( ShapedGlyphVec::const_iterator start );

		inline void addQuad( GlyphVertex * RESTRICT_ALIAS vertexBuffer,
							 Ogre::Vector2 topLeft,
							 Ogre::Vector2 bottomRight,
							 uint16_t glyphWidth,
							 uint16_t glyphHeight,
							 uint8_t *rgbaColour,
							 Ogre::Vector2 parentDerivedTL,
							 Ogre::Vector2 parentDerivedBR,
							 Ogre::Vector2 invSize,
							 uint32_t offset );

	public:
		Label( CrystalManager *manager );

		virtual bool isLabel() const		{ return true; }

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

		virtual void fillBuffersAndCommands( UiVertex * crystalgui_nonnull * crystalgui_nonnull
											 RESTRICT_ALIAS vertexBuffer,
											 GlyphVertex * crystalgui_nonnull * crystalgui_nonnull
											 RESTRICT_ALIAS textVertBuffer,
											 const Ogre::Vector2 &parentPos,
											 const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
