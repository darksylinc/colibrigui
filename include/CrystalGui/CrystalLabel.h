
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

		void updateGlyphs( States::States state );

	public:
		Label( CrystalManager *manager );

		/**
		@param text
			Text must be UTF8
		@param forState
			Use NumStates to affect all states
		*/
		void setText( const std::string &text, States::States forState=States::NumStates );

		virtual UiVertex* fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
												  const Ogre::Vector2 &parentPos,
												  const Ogre::Matrix3 &parentRot );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
