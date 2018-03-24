
#include "CrystalGui/CrystalLabel.h"
#include "CrystalGui/Text/CrystalShaperManager.h"

#include "CrystalRenderable.inl"

namespace Crystal
{
	Label::Label( CrystalManager *manager ) :
		Renderable( manager ),
		m_horizReadingDir( HorizReadingDir::Natural ),
		m_vertReadingDir( VertReadingDir::Disabled ),
		m_linebreakMode( LinebreakMode::CharWrap )
	{
		for( size_t i=0; i<States::NumStates; ++i )
			 m_glyphsDirty[i] = false;
	}
	//-------------------------------------------------------------------------
	void Label::updateGlyphs( States::States state )
	{
		ShaperManager *shaperManager = 0;

		{
			ShapedGlyphVec::const_iterator itor = m_shapes[state].begin();
			ShapedGlyphVec::const_iterator end  = m_shapes[state].end();

			while( itor != end )
			{
				shaperManager->releaseGlyph( itor->glyph );
				++itor;
			}

			m_shapes[state].clear();
		}

		//See if we can reuse the results from another state. If so,
		//we just need to copy them and increase the ref counts.
		bool reusableFound = false;
		for( size_t i=0; i<States::NumStates && !reusableFound; ++i )
		{
			if( i != state && !m_glyphsDirty[i] )
			{
				if( m_text[state] == m_text[i] && m_richText[state] == m_richText[i] )
				{
					m_shapes[state] = m_shapes[i];

					ShapedGlyphVec::const_iterator itor = m_shapes[state].begin();
					ShapedGlyphVec::const_iterator end  = m_shapes[state].end();

					while( itor != end )
					{
						shaperManager->addRefCount( itor->glyph );
						++itor;
					}

					reusableFound = true;
				}
			}
		}

		if( !reusableFound )
		{
			RichTextVec::const_iterator itor = m_richText[state].begin();
			RichTextVec::const_iterator end  = m_richText[state].end();
			while( itor != end )
			{
				const RichText &richText = *itor;
				const char *utf8Str = m_text[state].c_str() + richText.offset;
				shaperManager->renderString( utf8Str, richText, m_vertReadingDir, m_shapes[state] );
				++itor;
			}
		}
	}
	//-------------------------------------------------------------------------
	UiVertex* Label::fillBuffersAndCommands( UiVertex * RESTRICT_ALIAS vertexBuffer,
											 const Ogre::Vector2 &parentPos,
											 const Ogre::Matrix3 &parentRot )
	{
		if( !m_parent->intersectsChild( this ) )
			return vertexBuffer;

		Ogre::Vector2 caretPos = m_derivedTopLeft;

		const Ogre::Vector2 invWindowRes = m_manager->getInvWindowResolution();

		uint8_t rgbaColour[4];
		rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

		ShapedGlyphVec::const_iterator itor = m_shapes[m_currentState].begin();
		ShapedGlyphVec::const_iterator end  = m_shapes[m_currentState].end();

		while( itor != end )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			Ogre::Vector2 topLeft = caretPos + shapedGlyph.offset +
									Ogre::Vector2( shapedGlyph.glyph->bearingX,
												   shapedGlyph.glyph->bearingY );
			Ogre::Vector2 bottomRight = caretPos + Ogre::Vector2( shapedGlyph.glyph->width,
																  shapedGlyph.glyph->height ) *
										invWindowRes;

			Ogre::Vector4 uv( 1.0f );
			addQuad( vertexBuffer, topLeft, bottomRight, uv, rgbaColour,
					 Ogre::Vector2(1.0f),Ogre::Vector2(1.0f), Ogre::Vector2(1.0f) );
			vertexBuffer += 6u;

			caretPos += shapedGlyph.advance;

			++itor;
		}

		return vertexBuffer;
	}
	//-------------------------------------------------------------------------
	bool Label::_updateDirtyGlyphs()
	{
		bool retVal = false;
		for( size_t i=0; i<States::NumStates; ++i )
		{
			if( m_glyphsDirty[i] )
			{
				const size_t prevNumGlyphs = m_shapes[i].size();
				updateGlyphs( static_cast<States::States>( i ) );
				const size_t currNumGlyphs = m_shapes[i].size();

				if( prevNumGlyphs > currNumGlyphs )
					retVal = true;
			}
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	size_t Label::getMaxNumGlyphs() const
	{
		size_t retVal = 0;
		for( size_t i=0; i<States::NumStates; ++i )
			retVal = std::max( m_shapes[i].size(), retVal );

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Label::setText( const std::string &text, States::States forState )
	{
		if( forState == States::NumStates )
		{
			for( size_t i=0; i<States::NumStates; ++i )
			{
				if( m_text[i] != text )
				{
					m_text[i] = text;
					m_glyphsDirty[i] = true;
				}
			}
		}
		else
		{
			if( m_text[forState] != text )
			{
				m_text[forState] = text;
				m_glyphsDirty[forState] = true;
			}
		}
	}
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	bool RichText::operator == ( const RichText &other ) const
	{
		return	this->ptSize == other.ptSize &&
				this->offset == other.offset &&
				this->length == other.length &&
				this->font == other.font;
	}
}
