
#include "CrystalGui/CrystalLabel.h"
#include "CrystalGui/Text/CrystalShaperManager.h"

#include "CrystalRenderable.inl"

#include "OgreLwString.h"

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

		m_numVertices = 0;

		setCustomParameter( 6373, Ogre::Vector4( 1.0f ) );
	}
	//-------------------------------------------------------------------------
	void Label::validateRichText( States::States state )
	{
		const size_t textSize = m_text[state].size();
		if( m_richText[state].empty() )
		{
			RichText rt;
			rt.ptSize = 24u << 6u;
			rt.font = 0;
			rt.offset = 0;
			rt.length = textSize;
			m_richText[state].push_back( rt );
		}
		else
		{
			bool invalidRtDetected = false;
			RichTextVec::iterator itor = m_richText[state].begin();
			RichTextVec::iterator end  = m_richText[state].end();

			while( itor != end )
			{
				if( itor->offset > textSize )
				{
					itor->offset = textSize;
					invalidRtDetected = true;
				}
				if( itor->offset + itor->length > textSize )
				{
					itor->length = textSize - itor->offset;
					invalidRtDetected = true;
				}
				++itor;
			}

			if( invalidRtDetected )
			{
				LogListener *log = m_manager->getLogListener();
				char tmpBuffer[512];
				Ogre::LwString errorMsg( Ogre::LwString::FromEmptyPointer( tmpBuffer,
																		   sizeof(tmpBuffer) ) );

				errorMsg.clear();
				errorMsg.a( "[Label::validateRichText] Rich Edit goes out of bounds. "
							"We've corrected the situation. Text may not be drawn as expected."
							" String: ", m_text[state].c_str() );
				log->log( errorMsg.c_str(), LogSeverity::Warning );
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::updateGlyphs( States::States state )
	{
		ShaperManager *shaperManager = m_manager->getShaperManager();

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

		validateRichText( state );

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

		m_glyphsDirty[state] = false;
	}
	//-------------------------------------------------------------------------
	inline void Label::addQuad( GlyphVertex * RESTRICT_ALIAS vertexBuffer,
								Ogre::Vector2 topLeft,
								Ogre::Vector2 bottomRight,
								uint16_t glyphWidth,
								uint16_t glyphHeight,
								uint8_t *rgbaColour,
								Ogre::Vector2 parentDerivedTL,
								Ogre::Vector2 parentDerivedBR,
								Ogre::Vector2 invSize,
								uint32_t offset )
	{
		TODO_this_is_a_workaround_neg_y;
		#define CRYSTAL_ADD_VERTEX( _x, _y, _u, _v, clipDistanceTop, clipDistanceLeft, \
									clipDistanceRight, clipDistanceBottom ) \
			vertexBuffer->x = _x; \
			vertexBuffer->y = -_y; \
			vertexBuffer->width = glyphWidth; \
			vertexBuffer->height = glyphHeight; \
			vertexBuffer->offset = offset;\
			vertexBuffer->rgbaColour[0] = rgbaColour[0]; \
			vertexBuffer->rgbaColour[1] = rgbaColour[1]; \
			vertexBuffer->rgbaColour[2] = rgbaColour[2]; \
			vertexBuffer->rgbaColour[3] = rgbaColour[3]; \
			vertexBuffer->clipDistance[Borders::Top]	= clipDistanceTop; \
			vertexBuffer->clipDistance[Borders::Left]	= clipDistanceLeft; \
			vertexBuffer->clipDistance[Borders::Right]	= clipDistanceRight; \
			vertexBuffer->clipDistance[Borders::Bottom]	= clipDistanceBottom; \
			++vertexBuffer;

		CRYSTAL_ADD_VERTEX( topLeft.x, topLeft.y,
							0u, 0u,
							(topLeft.y - parentDerivedTL.y) * invSize.y,
							(topLeft.x - parentDerivedTL.x) * invSize.x,
							(parentDerivedBR.x - topLeft.x) * invSize.x,
							(parentDerivedBR.y - topLeft.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( topLeft.x, bottomRight.y,
							0u, glyphHeight,
							(bottomRight.y - parentDerivedTL.y) * invSize.y,
							(topLeft.x - parentDerivedTL.x) * invSize.x,
							(parentDerivedBR.x - topLeft.x) * invSize.x,
							(parentDerivedBR.y - bottomRight.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( bottomRight.x, bottomRight.y,
							glyphWidth, glyphHeight,
							(bottomRight.y - parentDerivedTL.y) * invSize.y,
							(bottomRight.x - parentDerivedTL.x) * invSize.x,
							(parentDerivedBR.x - bottomRight.x) * invSize.x,
							(parentDerivedBR.y - bottomRight.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( bottomRight.x, bottomRight.y,
							glyphWidth, glyphHeight,
							(bottomRight.y - parentDerivedTL.y) * invSize.y,
							(bottomRight.x - parentDerivedTL.x) * invSize.x,
							(parentDerivedBR.x - bottomRight.x) * invSize.x,
							(parentDerivedBR.y - bottomRight.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( bottomRight.x, topLeft.y,
							glyphWidth, 0u,
							(topLeft.y - parentDerivedTL.y) * invSize.y,
							(bottomRight.x - parentDerivedTL.x) * invSize.x,
							(parentDerivedBR.x - bottomRight.x) * invSize.x,
							(parentDerivedBR.y - topLeft.y) * invSize.y );

		CRYSTAL_ADD_VERTEX( topLeft.x, topLeft.y,
							0u, 0u,
							(topLeft.y - parentDerivedTL.y) * invSize.y,
							(topLeft.x - parentDerivedTL.x) * invSize.x,
							(parentDerivedBR.x - topLeft.x) * invSize.x,
							(parentDerivedBR.y - topLeft.y) * invSize.y );

		#undef CRYSTAL_ADD_VERTEX
	}
	//-------------------------------------------------------------------------
	void Label::fillBuffersAndCommands( UiVertex ** RESTRICT_ALIAS vertexBuffer,
										GlyphVertex ** RESTRICT_ALIAS _textVertBuffer,
										const Ogre::Vector2 &parentPos,
										const Ogre::Matrix3 &parentRot )
	{
		GlyphVertex * RESTRICT_ALIAS textVertBuffer = *_textVertBuffer;

		m_numVertices = 0;
		if( !m_parent->intersectsChild( this ) )
			return;

		updateDerivedTransform( parentPos, parentRot );

		const Ogre::Vector2 invWindowRes = m_manager->getInvWindowResolution2x();

		Ogre::Vector2 caretPos = m_derivedTopLeft;
		caretPos.y += m_shapes[m_currentState][0].glyph->height * invWindowRes.y;

		const Ogre::Vector2 parentDerivedTL = m_parent->m_derivedTopLeft;
		const Ogre::Vector2 parentDerivedBR = m_parent->m_derivedBottomRight;
		const Ogre::Vector2 invSize = 1.0f / (parentDerivedBR - parentDerivedTL);

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

			Ogre::Vector2 topLeft = caretPos +
									(shapedGlyph.offset +
									 Ogre::Vector2( shapedGlyph.glyph->bearingX,
													-shapedGlyph.glyph->bearingY )) * invWindowRes;
			Ogre::Vector2 bottomRight = Ogre::Vector2( caretPos.x + shapedGlyph.glyph->width *
													   invWindowRes.x,
													   topLeft.y + shapedGlyph.glyph->height *
													   invWindowRes.y );

			addQuad( textVertBuffer, topLeft, bottomRight,
					 shapedGlyph.glyph->width, shapedGlyph.glyph->height,
					 rgbaColour, parentDerivedTL, parentDerivedBR, invSize,
					 shapedGlyph.glyph->offsetStart );
			textVertBuffer += 6u;

			caretPos += shapedGlyph.advance * invWindowRes;

			m_numVertices += 6u;

			++itor;
		}

		*_textVertBuffer = textVertBuffer;
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

				if( currNumGlyphs > prevNumGlyphs )
					retVal = true;
			}
		}

		return retVal;
	}
	//-------------------------------------------------------------------------
	bool Label::isAnyStateDirty() const
	{
		bool retVal = false;
		for( size_t i=0; i<States::NumStates; ++i )
			retVal |= m_glyphsDirty[i];

		return retVal;
	}
	//-------------------------------------------------------------------------
	void Label::flagDirty( States::States state )
	{
		if( !isAnyStateDirty() )
			m_manager->_addDirtyLabel( this );
		m_glyphsDirty[state] = true;
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
					m_richText[i].clear();
					flagDirty( static_cast<States::States>( i ) );
				}
			}
		}
		else
		{
			if( m_text[forState] != text )
			{
				m_text[forState] = text;
				m_richText[forState].clear();
				flagDirty( forState );
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
