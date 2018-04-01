
#include "CrystalGui/CrystalLabel.h"
#include "CrystalGui/Text/CrystalShaperManager.h"

#include "CrystalRenderable.inl"

#include "OgreLwString.h"

namespace Crystal
{
	Label::Label( CrystalManager *manager ) :
		Renderable( manager ),
		m_horizAlignment( TextHorizAlignment::Natural ),
		m_vertAlignment( TextVertAlignment::Natural ),
		m_vertReadingDir( VertReadingDir::Disabled ),
		m_linebreakMode( LinebreakMode::WordWrap )
	{
		ShaperManager *shaperManager = m_manager->getShaperManager();
		for( size_t i=0; i<States::NumStates; ++i )
			m_actualHorizAlignment[i] = shaperManager->getDefaultTextDirection();

		for( size_t i=0; i<States::NumStates; ++i )
		{
			m_glyphsDirty[i] = false;
			m_glyphsPlaced[i] = true;
#if CRYSTALGUI_DEBUG_MEDIUM
			m_glyphsAligned[i] = true;
#endif
		}

		m_numVertices = 0;

		setCustomParameter( 6373, Ogre::Vector4( 1.0f ) );

		for( size_t i=0; i<States::NumStates; ++i )
			m_stateInformation[i].materialName = "## Crystal Default Text ##";

		setDatablock( manager->getDefaultTextDatablock() );
	}
	//-------------------------------------------------------------------------
	void Label::setTextHorizAlignment( TextHorizAlignment::TextHorizAlignment horizAlignment )
	{
		if( m_horizAlignment != horizAlignment )
		{
			m_horizAlignment = horizAlignment;
			for( size_t i=0; i<States::NumStates; ++i )
			{
				m_glyphsPlaced[i] = false;
#if CRYSTALGUI_DEBUG_MEDIUM
				m_glyphsAligned[i] = false;
#endif
			}
		}
	}
	//-------------------------------------------------------------------------
	TextHorizAlignment::TextHorizAlignment Label::getTextHorizAlignment() const
	{
		return m_horizAlignment;
	}
	//-------------------------------------------------------------------------
	void Label::validateRichText( States::States state )
	{
		const size_t textSize = m_text[state].size();
		if( m_richText[state].empty() )
		{
			RichText rt;
			rt.ptSize = 16u << 6u;
			rt.font = 0;
			rt.offset = 0;
			rt.length = textSize;
			rt.readingDir = HorizReadingDir::Default;
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
	void Label::updateGlyphs( States::States state, bool bPlaceGlyphs )
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
					m_glyphsPlaced[state] = m_glyphsPlaced[i];
#if CRYSTALGUI_DEBUG_MEDIUM
					m_glyphsAligned[state] = m_glyphsAligned[i];
#endif
					m_actualHorizAlignment[state] = m_actualHorizAlignment[i];

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
			bool alignmentUnknown = true;
			TextHorizAlignment::TextHorizAlignment actualHorizAlignment = TextHorizAlignment::Mixed;

			RichTextVec::const_iterator itor = m_richText[state].begin();
			RichTextVec::const_iterator end  = m_richText[state].end();
			while( itor != end )
			{
				const RichText &richText = *itor;
				const char *utf8Str = m_text[state].c_str() + richText.offset;
				TextHorizAlignment::TextHorizAlignment actualDir =
						shaperManager->renderString( utf8Str, richText, m_vertReadingDir,
													 m_shapes[state] );

				if( alignmentUnknown )
				{
					actualHorizAlignment = actualDir;
					alignmentUnknown = true;
				}
				else if( actualHorizAlignment != actualDir )
					actualHorizAlignment = TextHorizAlignment::Mixed;

				++itor;
			}

			if( m_horizAlignment == TextHorizAlignment::Natural )
			{
				if( actualHorizAlignment == TextHorizAlignment::Mixed )
					m_actualHorizAlignment[state] = shaperManager->getDefaultTextDirection();
				else
					m_actualHorizAlignment[state] = actualHorizAlignment;
			}
			else
				m_actualHorizAlignment[state] = m_horizAlignment;

		}

		m_glyphsDirty[state] = false;

		if( bPlaceGlyphs && !m_glyphsPlaced[state] )
			placeGlyphs( state );
	}
	//-------------------------------------------------------------------------
	void Label::placeGlyphs( States::States state, bool performAlignment )
	{
		const Ogre::Vector2 bottomRight = m_size * (2.0f * m_manager->getHalfWindowResolution() /
													m_manager->getCanvasSize());

		Word nextWord;
		memset( &nextWord, 0, sizeof(Word) );

		float largestHeight = findLineMaxHeight( m_shapes[state].begin(), state );
		nextWord.endCaretPos.y += largestHeight;

		bool multipleWordsInLine = false;

		while( findNextWord( nextWord, state ) )
		{
			if( m_linebreakMode == LinebreakMode::WordWrap )
			{
				float caretAtEndOfWord = nextWord.endCaretPos.x - nextWord.lastAdvance.x +
										 nextWord.lastCharWidth;
				float distBetweenWords = nextWord.endCaretPos.x - nextWord.startCaretPos.x;
				if( caretAtEndOfWord > bottomRight.x &&
					(distBetweenWords <= bottomRight.x || multipleWordsInLine) &&
					!m_shapes[state][nextWord.offset].isNewline )
				{
					float caretReturn = nextWord.startCaretPos.x;
					float wordLength = nextWord.endCaretPos.x - nextWord.startCaretPos.x;

					//Return to left.
					nextWord.startCaretPos.x -= caretReturn;
					nextWord.endCaretPos.x	 -= caretReturn;
					//Calculate alignment
					nextWord.startCaretPos.x	= 0.0f;
					nextWord.startCaretPos.y	+= largestHeight;
					nextWord.endCaretPos.x		= nextWord.startCaretPos.x + wordLength;
					nextWord.endCaretPos.y		= nextWord.startCaretPos.y;

					multipleWordsInLine = false;
				}
			}

			multipleWordsInLine = true;

			Ogre::Vector2 caretPos = nextWord.startCaretPos;

			ShapedGlyphVec::iterator itor = m_shapes[state].begin() + nextWord.offset;
			ShapedGlyphVec::iterator end  = itor + nextWord.length;

			while( itor != end )
			{
				ShapedGlyph &shapedGlyph = *itor;

				if( !shapedGlyph.isNewline && !shapedGlyph.isTab )
				{
					shapedGlyph.caretPos = caretPos;
					caretPos += shapedGlyph.advance;
				}
				else if( shapedGlyph.isTab )
				{
					// findNextWord already took care of this
				}
				else/* if( shapedGlyph.isNewline )*/
				{
					//Return to left. Newlines are zero width.
					nextWord.startCaretPos.x = 0.0f;
					nextWord.endCaretPos.x	 = 0.0f;
					//Calculate alignment
					nextWord.startCaretPos.x	= 0.0f;
					nextWord.startCaretPos.y	+= largestHeight;
					nextWord.endCaretPos		= nextWord.startCaretPos;
					multipleWordsInLine = false;
				}

				++itor;
			}
		}

		m_glyphsPlaced[state] = true;
#if CRYSTALGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = false;
#endif

		if( performAlignment )
			alignGlyphs( state );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Label::alignGlyphs( States::States state )
	{
		CRYSTAL_ASSERT_MEDIUM( !m_glyphsAligned[state] &&
							   "Calling alignGlyphs twice! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( (m_actualHorizAlignment[state] == TextHorizAlignment::Left ||
							 m_actualHorizAlignment[state] == TextHorizAlignment::Right ||
							 m_actualHorizAlignment[state] == TextHorizAlignment::Center) &&
							"m_actualHorizAlignment not set! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( m_glyphsPlaced[state] && "Did you call placeGlyphs?" );

		Ogre::Vector2 maxWidthHeight( Ogre::Vector2::ZERO );

		float lineWidth = 0;
		float prevCaretY = 0;

		const Ogre::Vector2 widgetBottomRight = m_size * (2.0f * m_manager->getHalfWindowResolution() /
														  m_manager->getCanvasSize());

		ShapedGlyphVec::iterator lineBegin = m_shapes[state].begin();
		ShapedGlyphVec::iterator itor = m_shapes[state].begin();
		ShapedGlyphVec::iterator end  = m_shapes[state].end();

		while( itor != end )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			float top = shapedGlyph.caretPos.y + shapedGlyph.offset.y + -shapedGlyph.glyph->bearingY;
			Ogre::Vector2 bottomRight = Ogre::Vector2( shapedGlyph.caretPos.x +
													   shapedGlyph.glyph->width,
													   top + shapedGlyph.glyph->height );

			if( (shapedGlyph.isNewline || prevCaretY != shapedGlyph.caretPos.y) &&
				m_actualHorizAlignment[state] != TextHorizAlignment::Left )
			{
				//Displace horizontally, the we line we just were in
				float newLeft = widgetBottomRight.x - lineWidth;
				if( m_actualHorizAlignment[state] == TextHorizAlignment::Center )
					newLeft *= 0.5f;

				while( lineBegin != itor )
				{
					lineBegin->caretPos.x += newLeft;
					++lineBegin;
				}

				prevCaretY	= shapedGlyph.caretPos.y;
				lineWidth	= 0;
			}

			lineWidth = Ogre::max( lineWidth, bottomRight.x );
			maxWidthHeight.makeCeil( bottomRight );

			++itor;
		}

		if( m_actualHorizAlignment[state] != TextHorizAlignment::Left )
		{
			//Displace horizontally, last line
			float newLeft = widgetBottomRight.x - lineWidth;
			if( m_actualHorizAlignment[state] == TextHorizAlignment::Center )
				newLeft *= 0.5f;

			while( lineBegin != end )
			{
				lineBegin->caretPos.x += newLeft;
				++lineBegin;
			}
		}

		if( m_vertAlignment != TextVertAlignment::Top && m_vertAlignment != TextVertAlignment::Natural )
		{
			//Iterate again, to vertically displace the entire string
			float newTop = widgetBottomRight.y - maxWidthHeight.y;
			if( m_vertAlignment == TextVertAlignment::Center )
				newTop *= 0.5f;

			itor = m_shapes[state].begin();
			while( itor != end )
			{
				lineBegin->caretPos.y += newTop;
				++itor;
			}
		}

#if CRYSTALGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = true;
#endif

		return maxWidthHeight;
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
	bool Label::findNextWord( Word &inOutWord, States::States state ) const
	{
		CRYSTAL_ASSERT_LOW( inOutWord.offset <= m_shapes[state].size() &&
							inOutWord.offset + inOutWord.length <= m_shapes[state].size() );

		Word word = inOutWord;

		if( word.offset == m_shapes[state].size() ||
			word.offset + word.length == m_shapes[state].size() )
		{
			word.length			= 0;
			word.lastAdvance	= 0;
			word.lastCharWidth	= 0;
			//word.endCaretPos		= Ogre::Vector2::ZERO;
			return false;
		}

		word.offset = word.offset + word.length;

		ShapedGlyphVec::const_iterator itor = m_shapes[state].begin() + word.offset;
		ShapedGlyphVec::const_iterator end  = m_shapes[state].end();

		ShapedGlyph firstGlyph = *itor;
		word.startCaretPos	= word.endCaretPos;
		word.endCaretPos	+= firstGlyph.advance;
		word.lastAdvance	= firstGlyph.advance;
		word.lastCharWidth	= firstGlyph.glyph->width;
		const bool isRtl	= firstGlyph.isRtl;
		++itor;

		if( !firstGlyph.isNewline && !firstGlyph.isWordBreaker )
		{
			while( itor != end && !itor->isNewline && !itor->isWordBreaker && itor->isRtl == isRtl )
			{
				const ShapedGlyph &shapedGlyph = *itor;
				word.endCaretPos	+= shapedGlyph.advance;
				word.lastAdvance	= shapedGlyph.advance;
				word.lastCharWidth	= shapedGlyph.glyph->width;
				++itor;
			}
		}
		else if( firstGlyph.isTab )
		{
			word.endCaretPos.x = ceilf( (word.startCaretPos.x +
										 firstGlyph.advance.x * 0.25f) /
										(firstGlyph.advance.x * 2.0f) ) *
								 (firstGlyph.advance.x * 2.0f);
		}

		word.length = itor - (m_shapes[state].begin() + word.offset);

		inOutWord = word;

		return word.length != 0;
	}
	//-------------------------------------------------------------------------
	float Label::findLineMaxHeight( ShapedGlyphVec::const_iterator start,
									States::States state ) const
	{
		CRYSTAL_ASSERT_LOW( start >= m_shapes[state].begin() &&
							start <= m_shapes[state].end() );

		float largestHeight = 0;

		ShapedGlyphVec::const_iterator itor = start;
		ShapedGlyphVec::const_iterator end  = m_shapes[state].end();
		while( itor != end && !itor->isNewline )
		{
			largestHeight = std::max( itor->glyph->newlineSize, largestHeight );
			++itor;
		}

		//The newline itself has its own height, make sure it's considered
		if( itor != end )
			largestHeight = std::max( itor->glyph->newlineSize, largestHeight );

		return largestHeight * 1.20f;
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

		const Ogre::Vector2 halfWindowRes = m_manager->getHalfWindowResolution();
		const Ogre::Vector2 invWindowRes = m_manager->getInvWindowResolution2x();

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

			if( !shapedGlyph.isNewline && !shapedGlyph.isTab )
			{
				Ogre::Vector2 topLeft = shapedGlyph.caretPos + shapedGlyph.offset +
										Ogre::Vector2( shapedGlyph.glyph->bearingX,
													   -shapedGlyph.glyph->bearingY );
				Ogre::Vector2 bottomRight = Ogre::Vector2( shapedGlyph.caretPos.x +
														   shapedGlyph.glyph->width,
														   topLeft.y + shapedGlyph.glyph->height );

				topLeft		= m_derivedTopLeft + topLeft * invWindowRes;
				bottomRight	= m_derivedTopLeft + bottomRight * invWindowRes;

				//Snap to pixels
				topLeft = topLeft * halfWindowRes;
				topLeft.x = roundf( topLeft.x );
				topLeft.y = roundf( topLeft.y );
				bottomRight = bottomRight * halfWindowRes;
				bottomRight.x = roundf( bottomRight.x );
				bottomRight.y = roundf( bottomRight.y );
				topLeft = topLeft * invWindowRes;
				bottomRight = bottomRight * invWindowRes;

				addQuad( textVertBuffer, topLeft, bottomRight,
						 shapedGlyph.glyph->width, shapedGlyph.glyph->height,
						 rgbaColour, parentDerivedTL, parentDerivedBR, invSize,
						 shapedGlyph.glyph->offsetStart );
				textVertBuffer += 6u;

				m_numVertices += 6u;
			}

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

			if( !m_glyphsPlaced[i] )
				placeGlyphs( static_cast<States::States>( i ) );
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
		m_glyphsPlaced[state] = false;
#if CRYSTALGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = false;
#endif
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
	void Label::sizeToFit( States::States baseState, float maxAllowedWidth,
						   TextHorizAlignment::TextHorizAlignment newHorizPos,
						   TextVertAlignment::TextVertAlignment newVertPos )
	{
		CRYSTAL_ASSERT_LOW( baseState < States::NumStates );

		if( m_glyphsDirty[baseState] )
			updateGlyphs( baseState, false );

		//Replace the glyphs forced to the Top-Left so we can gather the width & height
		const float oldWidth = m_size.x;
		m_size.x = maxAllowedWidth;
		placeGlyphs( baseState, false );
		m_size.x = oldWidth;

		//Gather width & height
		Ogre::Vector2 maxWidthHeight( Ogre::Vector2::ZERO );

		ShapedGlyphVec::iterator itor = m_shapes[baseState].begin();
		ShapedGlyphVec::iterator end  = m_shapes[baseState].end();

		while( itor != end )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			float top = shapedGlyph.caretPos.y + shapedGlyph.offset.y + -shapedGlyph.glyph->bearingY;
			Ogre::Vector2 bottomRight = Ogre::Vector2( shapedGlyph.caretPos.x +
													   shapedGlyph.glyph->width,
													   top + shapedGlyph.glyph->height );
			maxWidthHeight.makeCeil( bottomRight );
			++itor;
		}

		//Set new dimensions
		const Ogre::Vector2 canvasSize = m_manager->getCanvasSize();
		const Ogre::Vector2 invWindowRes = 0.5f * m_manager->getInvWindowResolution2x();

		Ogre::Vector2 oldSize = m_size;
		m_size = maxWidthHeight * invWindowRes * canvasSize;

		//Align the glyphs so horizontal & vertical alignment are respected
		alignGlyphs( baseState );

		//Now reposition the widget based on input newHorizPos & newVertPos
		if( newHorizPos == TextHorizAlignment::Natural )
			newHorizPos = m_actualHorizAlignment[baseState];
		if( newVertPos == TextVertAlignment::Natural )
			newVertPos = TextVertAlignment::Top;

		switch( newHorizPos )
		{
		case TextHorizAlignment::Natural:
		case TextHorizAlignment::Left:
			break;
		case TextHorizAlignment::Center:
			m_position.x = (m_position.x + oldSize.x - maxWidthHeight.x) * 0.5f;
			break;
		case TextHorizAlignment::Right:
			m_position.x = m_position.x + oldSize.x - maxWidthHeight.x;
			break;
		}
		switch( newHorizPos )
		{
		case TextVertAlignment::Natural:
		case TextVertAlignment::Top:
			break;
		case TextVertAlignment::Center:
			m_position.y = (m_position.y + oldSize.y - maxWidthHeight.y) * 0.5f;
			break;
		case TextVertAlignment::Bottom:
			m_position.y = m_position.y + oldSize.y - maxWidthHeight.y;
			break;
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
