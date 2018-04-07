
#include "CrystalGui/CrystalLabel.h"
#include "CrystalGui/Text/CrystalShaperManager.h"

#include "CrystalRenderable.inl"

#include "OgreLwString.h"

namespace Crystal
{
	inline void getCorners( const ShapedGlyph &shapedGlyph,
							Ogre::Vector2 &topLeft, Ogre::Vector2 &bottomRight )
	{
		topLeft = shapedGlyph.caretPos + shapedGlyph.offset +
								Ogre::Vector2( shapedGlyph.glyph->bearingX,
											   -shapedGlyph.glyph->bearingY );
		bottomRight = Ogre::Vector2( topLeft.x + shapedGlyph.glyph->width,
									 topLeft.y + shapedGlyph.glyph->height );
	}

	Label::Label( CrystalManager *manager ) :
		Renderable( manager ),
		m_usesBackground( false ),
		m_shadowOutline( false ),
		m_shadowColour( Ogre::ColourValue::Black ),
		m_shadowDisplace( 1.0f ),
		m_backgroundSize( Ogre::Vector2::ZERO ),
		m_defaultBackgroundColour( Ogre::ColourValue( 0.0f, 0.0f, 0.0f, 0.5f ) ),
		m_linebreakMode( LinebreakMode::WordWrap ),
		m_horizAlignment( TextHorizAlignment::Natural ),
		m_vertAlignment( TextVertAlignment::Natural ),
		m_vertReadingDir( VertReadingDir::Disabled )
	{
		ShaperManager *shaperManager = m_manager->getShaperManager();
		for( size_t i=0; i<States::NumStates; ++i )
		{
			m_actualHorizAlignment[i] = shaperManager->getDefaultTextDirection();
			m_actualVertReadingDir[i] = VertReadingDir::Disabled;
		}

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
	void Label::setTextVertAlignment( TextVertAlignment::TextVertAlignment vertAlignment )
	{
		if( m_vertAlignment != vertAlignment )
		{
			m_vertAlignment = vertAlignment;
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
	void Label::setVertReadingDir( VertReadingDir::VertReadingDir vertReadingDir )
	{
		if( m_vertReadingDir != vertReadingDir )
		{
			m_vertReadingDir = vertReadingDir;
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
	VertReadingDir::VertReadingDir Label::getVertReadingDir() const
	{
		return m_vertReadingDir;
	}
	//-------------------------------------------------------------------------
	void Label::setShadowOutline( bool enable, Ogre::ColourValue shadowColour,
								  const Ogre::Vector2 &shadowDisplace )
	{
		m_shadowOutline = enable;
		m_shadowColour = shadowColour;
		m_shadowDisplace = shadowDisplace;
	}
	//-------------------------------------------------------------------------
	void Label::validateRichText( States::States state )
	{
		const size_t textSize = m_text[state].size();
		if( m_richText[state].empty() )
		{
			RichText rt;
			rt.ptSize = 16u << 6u;
			rt.rgba32 = m_colour.getAsABGR();
			rt.noBackground = true;
			rt.backgroundRgba32 = m_defaultBackgroundColour.getAsABGR();
			rt.font = 0;
			rt.offset = 0;
			rt.length = textSize;
			rt.readingDir = HorizReadingDir::Default;
			rt.glyphStart = rt.glyphEnd = 0;
			m_richText[state].push_back( rt );

			m_usesBackground = false;
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

				m_usesBackground |= !itor->noBackground;

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
					m_actualVertReadingDir[state] = m_actualVertReadingDir[i];

					ShapedGlyphVec::const_iterator itor = m_shapes[state].begin();
					ShapedGlyphVec::const_iterator end  = m_shapes[state].end();

					while( itor != end )
					{
						shaperManager->addRefCount( itor->glyph );
						++itor;
					}

					const size_t numRichText = m_richText[state].size();
					for( size_t j=0; j<numRichText; ++j )
					{
						m_richText[state][j].glyphStart = m_richText[i][j].glyphStart;
						m_richText[state][j].glyphEnd = m_richText[i][j].glyphEnd;
					}

					reusableFound = true;
				}
			}
		}

		if( !reusableFound )
		{
			bool alignmentUnknown = true;
			TextHorizAlignment::TextHorizAlignment actualHorizAlignment = TextHorizAlignment::Mixed;

			RichTextVec::iterator itor = m_richText[state].begin();
			RichTextVec::iterator end  = m_richText[state].end();
			while( itor != end )
			{
				RichText &richText = *itor;
				richText.glyphStart = m_shapes[state].size();
				const char *utf8Str = m_text[state].c_str() + richText.offset;
				TextHorizAlignment::TextHorizAlignment actualDir =
						shaperManager->renderString( utf8Str, richText, itor - m_richText[state].begin(),
													 m_vertReadingDir, m_shapes[state] );
				richText.glyphEnd = m_shapes[state].size();

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
				if( m_vertReadingDir == VertReadingDir::ForceTTB )
					m_actualHorizAlignment[state] = TextHorizAlignment::Right;
				else if( m_vertReadingDir == VertReadingDir::ForceTTBLTR )
					m_actualHorizAlignment[state] = TextHorizAlignment::Left;
				else if( actualHorizAlignment == TextHorizAlignment::Mixed )
					m_actualHorizAlignment[state] = shaperManager->getDefaultTextDirection();
				else
					m_actualHorizAlignment[state] = actualHorizAlignment;
			}
			else
				m_actualHorizAlignment[state] = m_horizAlignment;

			if( m_vertReadingDir != VertReadingDir::Disabled )
			{
				if( m_vertReadingDir == VertReadingDir::ForceTTB ||
					m_vertReadingDir == VertReadingDir::ForceTTBLTR )
				{
					m_actualVertReadingDir[state] = m_vertReadingDir;
				}
				else
					m_actualVertReadingDir[state] = shaperManager->getPreferredVertReadingDir();
			}
			else
				m_actualVertReadingDir[state] = m_vertReadingDir;
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
		if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			nextWord.endCaretPos.y += largestHeight;
		else
			nextWord.endCaretPos.x += largestHeight * 0.5f;

		bool multipleWordsInLine = false;

		const float vertReadDirSign =
				m_actualVertReadingDir[state] == VertReadingDir::ForceTTB ? -1.0f : 1.0f;

		while( findNextWord( nextWord, state ) )
		{
			if( m_linebreakMode == LinebreakMode::WordWrap )
			{
				if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
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
				else
				{
					float caretAtEndOfWord = nextWord.endCaretPos.y - nextWord.lastAdvance.y +
											 nextWord.lastCharWidth;
					float distBetweenWords = nextWord.endCaretPos.y - nextWord.startCaretPos.y;
					if( caretAtEndOfWord > bottomRight.y &&
						(distBetweenWords <= bottomRight.y || multipleWordsInLine) &&
						!m_shapes[state][nextWord.offset].isNewline )
					{
						float caretReturn = nextWord.startCaretPos.y;
						float wordLength = nextWord.endCaretPos.y - nextWord.startCaretPos.y;

						//Return to top.
						nextWord.startCaretPos.y -= caretReturn;
						nextWord.endCaretPos.y	 -= caretReturn;
						//Calculate alignment
						nextWord.startCaretPos.x += largestHeight * vertReadDirSign;
						nextWord.startCaretPos.y	= 0.0f;
						nextWord.endCaretPos.x		= nextWord.startCaretPos.x;
						nextWord.endCaretPos.y		= nextWord.startCaretPos.y + wordLength;
						multipleWordsInLine = false;
					}
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
					shapedGlyph.caretPos = caretPos;
					caretPos += shapedGlyph.advance;
				}
				else/* if( shapedGlyph.isNewline )*/
				{
					largestHeight = findLineMaxHeight( itor + 1u, state );

					if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
					{
						//Return to left. Newlines are zero width.
						nextWord.startCaretPos.x = 0.0f;
						nextWord.endCaretPos.x	 = 0.0f;
						//Calculate alignment
						nextWord.startCaretPos.x	= 0.0f;
						nextWord.startCaretPos.y	+= largestHeight;
					}
					else
					{
						//Return to top. Newlines are zero width.
						nextWord.startCaretPos.y = 0.0f;
						nextWord.endCaretPos.y	 = 0.0f;
						//Calculate alignment
						nextWord.startCaretPos.x	+= largestHeight * vertReadDirSign;
						nextWord.startCaretPos.y	= 0.0f;
					}
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
	void Label::alignGlyphs( States::States state )
	{
		if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			alignGlyphsHorizReadingDir( state );
		else
			alignGlyphsVertReadingDir( state );
	}
	//-------------------------------------------------------------------------
	void Label::alignGlyphsHorizReadingDir( States::States state )
	{
		CRYSTAL_ASSERT_MEDIUM( !m_glyphsAligned[state] &&
							   "Calling alignGlyphs twice! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( (m_actualHorizAlignment[state] == TextHorizAlignment::Left ||
							 m_actualHorizAlignment[state] == TextHorizAlignment::Right ||
							 m_actualHorizAlignment[state] == TextHorizAlignment::Center) &&
							"m_actualHorizAlignment not set! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( m_glyphsPlaced[state] && "Did you call placeGlyphs?" );

		if( m_actualHorizAlignment[state] == TextHorizAlignment::Left &&
			m_vertAlignment == TextVertAlignment::Top )
		{
			//Nothing to align
			return;
		}

		float lineWidth = 0;
		float prevCaretY = 0;

		const Ogre::Vector2 widgetBottomRight = m_size * (2.0f * m_manager->getHalfWindowResolution() /
														  m_manager->getCanvasSize());

		//To gather width & height
		Ogre::Vector2 maxBottomRight( -std::numeric_limits<float>::max() );
		Ogre::Vector2 minTopLeft( std::numeric_limits<float>::max() );

		ShapedGlyphVec::iterator lineBegin = m_shapes[state].begin();
		ShapedGlyphVec::iterator itor = m_shapes[state].begin();
		ShapedGlyphVec::iterator end  = m_shapes[state].end();

		while( itor != end )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			Ogre::Vector2 topLeft, bottomRight;
			getCorners( shapedGlyph, topLeft, bottomRight );

			if( (shapedGlyph.isNewline || prevCaretY != shapedGlyph.caretPos.y) &&
				m_actualHorizAlignment[state] != TextHorizAlignment::Left )
			{
				//Displace horizontally, the line we just were in
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
			minTopLeft.makeFloor( topLeft );
			maxBottomRight.makeCeil( bottomRight );

			++itor;
		}

		const Ogre::Vector2 maxWidthHeight( maxBottomRight - minTopLeft );

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
				itor->caretPos.y += newTop;
				++itor;
			}
		}

#if CRYSTALGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = true;
#endif
	}
	//-------------------------------------------------------------------------
	void Label::alignGlyphsVertReadingDir( States::States state )
	{
		CRYSTAL_ASSERT_MEDIUM( !m_glyphsAligned[state] &&
							   "Calling alignGlyphs twice! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( (m_actualHorizAlignment[state] == TextHorizAlignment::Left ||
							 m_actualHorizAlignment[state] == TextHorizAlignment::Right ||
							 m_actualHorizAlignment[state] == TextHorizAlignment::Center) &&
							"m_actualHorizAlignment not set! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( (m_actualVertReadingDir[state] == VertReadingDir::ForceTTB ||
							 m_actualVertReadingDir[state] == VertReadingDir::ForceTTBLTR) &&
							"m_actualVertReadingDir not set! updateGlyphs not called?" );
		CRYSTAL_ASSERT_LOW( m_glyphsPlaced[state] && "Did you call placeGlyphs?" );

		if( m_vertAlignment == TextVertAlignment::Top &&
			m_actualHorizAlignment[state] == TextHorizAlignment::Left &&
			m_actualVertReadingDir[state] == VertReadingDir::ForceTTBLTR )
		{
			//Nothing to align
			return;
		}

		float lineWidth = 0;
		float prevCaretX = 0;

		const Ogre::Vector2 widgetBottomRight = m_size * (2.0f * m_manager->getHalfWindowResolution() /
														  m_manager->getCanvasSize());

		//To gather width & height
		Ogre::Vector2 maxBottomRight( -std::numeric_limits<float>::max() );
		Ogre::Vector2 minTopLeft( std::numeric_limits<float>::max() );

		ShapedGlyphVec::iterator lineBegin = m_shapes[state].begin();
		ShapedGlyphVec::iterator itor = m_shapes[state].begin();
		ShapedGlyphVec::iterator end  = m_shapes[state].end();

		while( itor != end )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			Ogre::Vector2 topLeft, bottomRight;
			getCorners( shapedGlyph, topLeft, bottomRight );

			if( (shapedGlyph.isNewline || prevCaretX != shapedGlyph.caretPos.x) &&
				m_vertAlignment > TextVertAlignment::Top )
			{
				//Displace horizontally, the line we just were in
				float newTop = widgetBottomRight.y - lineWidth;
				if( m_vertAlignment == TextVertAlignment::Center )
					newTop *= 0.5f;

				while( lineBegin != itor )
				{
					lineBegin->caretPos.y += newTop;
					++lineBegin;
				}

				prevCaretX	= shapedGlyph.caretPos.x;
				lineWidth	= 0;
			}

			lineWidth = Ogre::max( lineWidth, bottomRight.y );
			minTopLeft.makeFloor( topLeft );
			maxBottomRight.makeCeil( bottomRight );

			++itor;
		}

		if( m_vertAlignment > TextVertAlignment::Top )
		{
			//Displace vertically, last line
			float newTop = widgetBottomRight.y - lineWidth;
			if( m_vertAlignment == TextVertAlignment::Center )
				newTop *= 0.5f;

			while( lineBegin != end )
			{
				lineBegin->caretPos.y += newTop;
				++lineBegin;
			}
		}

		const Ogre::Vector2 maxWidthHeight( maxBottomRight - minTopLeft );

		if( m_actualHorizAlignment[state] != TextHorizAlignment::Left ||
			m_actualVertReadingDir[state] == VertReadingDir::ForceTTB )
		{
			//Iterate again, to horizontally displace the entire string
			float newLeft = widgetBottomRight.x - maxWidthHeight.x;
			if( m_actualHorizAlignment[state] == TextHorizAlignment::Center )
				newLeft *= 0.5f;
			else if( m_actualHorizAlignment[state] == TextHorizAlignment::Left &&
					 m_actualVertReadingDir[state] == VertReadingDir::ForceTTB )
			{
				newLeft = maxWidthHeight.x;
			}

			itor = m_shapes[state].begin();
			while( itor != end )
			{
				itor->caretPos.x += newLeft;
				++itor;
			}
		}

#if CRYSTALGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = true;
#endif
	}
	//-------------------------------------------------------------------------
	inline void Label::addQuad( GlyphVertex * RESTRICT_ALIAS vertexBuffer,
								Ogre::Vector2 topLeft,
								Ogre::Vector2 bottomRight,
								uint16_t glyphWidth,
								uint16_t glyphHeight,
								uint32_t rgbaColour,
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
			vertexBuffer->rgbaColour = rgbaColour; \
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
		if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			word.lastCharWidth	= firstGlyph.glyph->width;
		else
			word.lastCharWidth	= firstGlyph.glyph->height;
		const bool isRtl	= firstGlyph.isRtl;
		++itor;

		if( !firstGlyph.isNewline && !firstGlyph.isWordBreaker )
		{
			while( itor != end && !itor->isNewline && !itor->isWordBreaker && itor->isRtl == isRtl )
			{
				const ShapedGlyph &shapedGlyph = *itor;
				word.endCaretPos	+= shapedGlyph.advance;
				word.lastAdvance	= shapedGlyph.advance;
				if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
					word.lastCharWidth	= shapedGlyph.glyph->width;
				else
					word.lastCharWidth	= shapedGlyph.glyph->height;
				++itor;
			}
		}
		else if( firstGlyph.isTab )
		{
			if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			{
				word.endCaretPos.x = ceilf( (word.startCaretPos.x +
											 firstGlyph.advance.x * 0.25f) /
											(firstGlyph.advance.x * 2.0f) ) *
									 (firstGlyph.advance.x * 2.0f);
			}
			else
			{
				word.endCaretPos.y = ceilf( (word.startCaretPos.y +
											 firstGlyph.advance.y * 0.25f) /
											(firstGlyph.advance.y * 4.0f) ) *
									 (firstGlyph.advance.y * 4.0f);
			}
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

		return largestHeight;
	}
	//-------------------------------------------------------------------------
	GlyphVertex* Label::fillBackground( GlyphVertex * RESTRICT_ALIAS textVertBuffer,
										const Ogre::Vector2 halfWindowRes,
										const Ogre::Vector2 invWindowRes,
										const Ogre::Vector2 parentDerivedTL,
										const Ogre::Vector2 parentDerivedBR,
										const bool isHorizontal )
	{
		const Ogre::Vector2 invSize = 1.0f / (parentDerivedBR - parentDerivedTL);

		RichTextVec::const_iterator itRichText = m_richText[m_currentState].begin();
		RichTextVec::const_iterator enRichText = m_richText[m_currentState].end();

		while( itRichText != enRichText )
		{
			if( !itRichText->noBackground )
			{
				float prevCaretY = 0;

				const uint32_t backgroundColour = itRichText->backgroundRgba32;
				const Ogre::Vector2 backgroundDisplacement = invWindowRes * m_backgroundSize;

				float lineHeight = 0;
				float mostTop	= std::numeric_limits<float>::max();
				float mostLeft	= std::numeric_limits<float>::max();
				float mostRight	= -std::numeric_limits<float>::max();
				float mostBottom= -std::numeric_limits<float>::max();

				ShapedGlyphVec::const_iterator itor = m_shapes[m_currentState].begin() +
													  itRichText->glyphStart;
				ShapedGlyphVec::const_iterator end  = m_shapes[m_currentState].begin() +
													  itRichText->glyphEnd;

				if( itor != end )
				{
					if( isHorizontal )
						prevCaretY = itor->caretPos.y;
					else
						prevCaretY = itor->caretPos.x;
				}

				while( itor != end )
				{
					const ShapedGlyph &shapedGlyph = *itor;

					const bool changesLine = shapedGlyph.isNewline ||
											 (prevCaretY != shapedGlyph.caretPos.y && isHorizontal) ||
											 (prevCaretY != shapedGlyph.caretPos.x && !isHorizontal);

					if( !shapedGlyph.isNewline && !changesLine )
					{
						lineHeight = Ogre::max( lineHeight, shapedGlyph.glyph->newlineSize );
						mostTop = Ogre::min( mostTop, shapedGlyph.caretPos.y );
						mostBottom = Ogre::max( mostBottom, shapedGlyph.caretPos.y );
						if( isHorizontal )
						{
							mostLeft = Ogre::min( mostLeft, shapedGlyph.caretPos.x +
												  shapedGlyph.offset.x +
												  shapedGlyph.glyph->bearingX );
							mostRight = Ogre::max( mostRight, shapedGlyph.caretPos.x +
												   shapedGlyph.offset.x +
												   shapedGlyph.glyph->bearingX +
												   shapedGlyph.glyph->width );
						}
						else
						{
							mostLeft = Ogre::min( mostLeft, shapedGlyph.caretPos.x -
												  lineHeight * 0.5f );
							mostRight = Ogre::max( mostRight, shapedGlyph.caretPos.x +
												   lineHeight * 0.5f );
						}
					}

					if( itor + 1u == end || changesLine )
					{
						const float regionUp = isHorizontal ? shapedGlyph.glyph->regionUp : 0.0f;

						//New line found. Render the background and reset the counters
						Ogre::Vector2 topLeft =
								Ogre::Vector2( mostLeft, mostTop - lineHeight * regionUp );
						Ogre::Vector2 bottomRight =
								Ogre::Vector2( mostRight, mostBottom + lineHeight * (1.0f - regionUp) );
						topLeft		= m_derivedTopLeft + topLeft * invWindowRes;
						bottomRight	= m_derivedTopLeft + bottomRight * invWindowRes;

						//Snap to pixels
						topLeft = topLeft * halfWindowRes;
						topLeft.x = ceilf( topLeft.x );
						topLeft.y = ceilf( topLeft.y );
						bottomRight = bottomRight * halfWindowRes;
						bottomRight.x = ceilf( bottomRight.x );
						bottomRight.y = ceilf( bottomRight.y );
						topLeft = topLeft * invWindowRes;
						bottomRight = bottomRight * invWindowRes;

						addQuad( textVertBuffer,
								 topLeft - backgroundDisplacement,
								 bottomRight + backgroundDisplacement,
								 1, 1,
								 backgroundColour, parentDerivedTL, parentDerivedBR, invSize,
								 0 );
						textVertBuffer += 6u;
						m_numVertices += 6u;

						Ogre::Vector2 nextCaret = shapedGlyph.caretPos;
						if( shapedGlyph.isNewline && itor + 1u != end )
							nextCaret = (itor + 1u)->caretPos;

						if( !isHorizontal )
							prevCaretY	= nextCaret.x;
						else
							prevCaretY	= nextCaret.y;
						lineHeight = 0;
						mostTop		= std::numeric_limits<float>::max();
						mostLeft	= std::numeric_limits<float>::max();
						mostRight	= -std::numeric_limits<float>::max();
						mostBottom	= -std::numeric_limits<float>::max();

						//Step backwards and reprocess this glyph again,
						//as part of the new line and not the current one.
						if( !shapedGlyph.isNewline && changesLine )
							--itor;
					}

					++itor;
				}
			}

			++itRichText;
		}

		return textVertBuffer;
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

		const uint32_t shadowColour = m_shadowColour.getAsABGR();

		const Ogre::Vector2 halfWindowRes = m_manager->getHalfWindowResolution();
		const Ogre::Vector2 invWindowRes = m_manager->getInvWindowResolution2x();

		const Ogre::Vector2 shadowDisplacement = invWindowRes * m_shadowDisplace;

		const Ogre::Vector2 parentDerivedTL = m_parent->m_derivedTopLeft;
		const Ogre::Vector2 parentDerivedBR = m_parent->m_derivedBottomRight;
		const Ogre::Vector2 invSize = 1.0f / (parentDerivedBR - parentDerivedTL);

		if( m_usesBackground )
		{
			const bool isHoriz = m_actualVertReadingDir[m_currentState] == VertReadingDir::Disabled;
			textVertBuffer = fillBackground( textVertBuffer, halfWindowRes, invWindowRes,
											 parentDerivedTL, parentDerivedBR, isHoriz );
		}

		ShapedGlyphVec::const_iterator itor = m_shapes[m_currentState].begin();
		ShapedGlyphVec::const_iterator end  = m_shapes[m_currentState].end();

		while( itor != end )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			if( !shapedGlyph.isNewline && !shapedGlyph.isTab )
			{
				Ogre::Vector2 topLeft, bottomRight;
				getCorners( shapedGlyph, topLeft, bottomRight );

				topLeft		= m_derivedTopLeft + topLeft * invWindowRes;
				bottomRight	= m_derivedTopLeft + bottomRight * invWindowRes;

				//Snap to pixels
				topLeft = topLeft * halfWindowRes;
				topLeft.x = ceilf( topLeft.x );
				topLeft.y = ceilf( topLeft.y );
				bottomRight = bottomRight * halfWindowRes;
				bottomRight.x = ceilf( bottomRight.x );
				bottomRight.y = ceilf( bottomRight.y );
				topLeft = topLeft * invWindowRes;
				bottomRight = bottomRight * invWindowRes;

				if( m_shadowOutline )
				{
					addQuad( textVertBuffer,
							 topLeft + shadowDisplacement,
							 bottomRight + shadowDisplacement,
							 shapedGlyph.glyph->width, shapedGlyph.glyph->height,
							 shadowColour, parentDerivedTL, parentDerivedBR, invSize,
							 shapedGlyph.glyph->offsetStart );
					textVertBuffer += 6u;
					m_numVertices += 6u;
				}

				const RichText &richText = m_richText[m_currentState][shapedGlyph.richTextIdx];

				addQuad( textVertBuffer, topLeft, bottomRight,
						 shapedGlyph.glyph->width, shapedGlyph.glyph->height,
						 richText.rgba32, parentDerivedTL, parentDerivedBR, invSize,
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
		m_usesBackground = false;
	}
	//-------------------------------------------------------------------------
	size_t Label::getMaxNumGlyphs() const
	{
		size_t retVal = 0;
		for( size_t i=0; i<States::NumStates; ++i )
			retVal = std::max( m_shapes[i].size(), retVal );

		const size_t maxGlyphs = retVal;

		if( m_shadowOutline )
			retVal += maxGlyphs;
		if( m_usesBackground )
			retVal += maxGlyphs;

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

		if( m_shapes[baseState].empty() )
			return;

		//Replace the glyphs forced to the Top-Left so we can gather the width & height
		const float oldWidth = m_size.x;
		m_size.x = maxAllowedWidth;
		placeGlyphs( baseState, false );
		m_size.x = oldWidth;

		//Gather width & height
		Ogre::Vector2 maxBottomRight( -std::numeric_limits<float>::max() );
		Ogre::Vector2 minTopLeft( std::numeric_limits<float>::max() );

		ShapedGlyphVec::iterator itor = m_shapes[baseState].begin();
		ShapedGlyphVec::iterator end  = m_shapes[baseState].end();

		while( itor != end )
		{
			Ogre::Vector2 topLeft, bottomRight;
			getCorners( *itor, topLeft, bottomRight );
			minTopLeft.makeFloor( topLeft );
			maxBottomRight.makeCeil( bottomRight );
			++itor;
		}

		const Ogre::Vector2 maxWidthHeight( maxBottomRight - minTopLeft );

		if( maxWidthHeight.x < 0 || maxWidthHeight.y < 0 )
			return;

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
			m_position.x = (m_position.x + oldSize.x - m_size.x) * 0.5f;
			break;
		case TextHorizAlignment::Right:
			m_position.x = m_position.x + oldSize.x - m_size.x;
			break;
		}
		switch( newVertPos )
		{
		case TextVertAlignment::Natural:
		case TextVertAlignment::Top:
			break;
		case TextVertAlignment::Center:
			m_position.y = (m_position.y + oldSize.y - m_size.y) * 0.5f;
			break;
		case TextVertAlignment::Bottom:
			m_position.y = m_position.y + oldSize.y - m_size.y;
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
				this->readingDir == other.readingDir &&
				this->font == other.font;
	}
}
