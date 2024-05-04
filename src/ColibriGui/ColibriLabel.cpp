
#include "ColibriGui/ColibriLabel.h"

#include "ColibriGui/ColibriLabelBmp.h"
#include "ColibriGui/Text/ColibriBmpFont.h"
#include "ColibriGui/Text/ColibriShaperManager.h"
#include "ColibriRenderable.inl"

#include "OgreLwString.h"

#include "unicode/unistr.h"

namespace Colibri
{
	inline void getCorners( const ShapedGlyph &shapedGlyph, Ogre::Vector2 &topLeft,
							Ogre::Vector2 &bottomRight )
	{
		topLeft = shapedGlyph.caretPos + shapedGlyph.offset +
				  Ogre::Vector2( shapedGlyph.glyph->bearingX, -shapedGlyph.glyph->bearingY );
		bottomRight =
			Ogre::Vector2( topLeft.x + shapedGlyph.glyph->width, topLeft.y + shapedGlyph.glyph->height );
	}

	inline void getAlignmentCorners( const ShapedGlyph &shapedGlyph, Ogre::Vector2 &topLeft,
									 Ogre::Vector2 &bottomRight )
	{
		topLeft = shapedGlyph.caretPos + shapedGlyph.offset +
				  Ogre::Vector2( shapedGlyph.glyph->bearingX, -shapedGlyph.glyph->bearingY );
		bottomRight =
			Ogre::Vector2( topLeft.x + shapedGlyph.glyph->width, topLeft.y + shapedGlyph.glyph->height );

		const Ogre::Vector2 nextTopLeft = shapedGlyph.caretPos + shapedGlyph.advance;
		bottomRight.makeCeil( nextTopLeft );
	}

	Label::Label( ColibriManager *manager ) :
		Renderable( manager ),
		m_usesBackground( false ),
		m_clipTextToWidget( true ),
		m_shadowOutline( false ),
		m_shadowColour( Ogre::ColourValue::Black ),
		m_shadowDisplace( 1.0f ),
		m_defaultColour( m_colour ),
		m_backgroundSize( Ogre::Vector2::ZERO ),
		m_defaultBackgroundColour( Ogre::ColourValue( 0.0f, 0.0f, 0.0f, 0.5f ) ),
		m_defaultFontSize( m_manager->getDefaultFontSize26d6() ),
		m_defaultFont( 0 ),
		m_lineHeightScale( 1.0f ),
		m_lastLineHeightScale( 1.0f ),
		m_linebreakMode( LinebreakMode::WordWrap ),
		m_horizAlignment( TextHorizAlignment::Natural ),
		m_vertAlignment( TextVertAlignment::Natural ),
		m_vertReadingDir( VertReadingDir::Disabled ),
		m_rasterPrivateArea( 0 )
	{
		m_overrideSkinColour = true;
		setVao( m_manager->getTextVao() );

		ShaperManager *shaperManager = m_manager->getShaperManager();
		for( size_t i = 0; i < States::NumStates; ++i )
		{
			m_actualHorizAlignment[i] = shaperManager->getDefaultTextDirection();
			m_actualVertReadingDir[i] = VertReadingDir::Disabled;
		}

		for( size_t i = 0; i < States::NumStates; ++i )
		{
			m_glyphsDirty[i] = false;
			m_glyphsPlaced[i] = true;
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
			m_glyphsAligned[i] = true;
#endif
		}

		m_numVertices = 0;

		setCustomParameter( 6373, Ogre::Vector4( 1.0f ) );

		for( size_t i = 0; i < States::NumStates; ++i )
			m_stateInformation[i].materialName = ColibriManager::c_defaultTextDatablockNames[i];

		Ogre::HlmsDatablock *datablock = manager->getDefaultTextDatablock()[States::Idle];
		COLIBRI_ASSERT_MEDIUM(
			datablock &&
			"getDefaultTextDatablock returned false. Please call setOgre first, and ensure the "
			"ShaperManager (fonts) has already been properly initialized" );
		setDatablock( datablock );
	}
	//-------------------------------------------------------------------------
	void Label::_destroy()
	{
		Renderable::_destroy();

		// Rasters are children of us, so it will be destroyed by our super class
		m_rasterPrivateArea = 0;
	}
	//-------------------------------------------------------------------------
	Label::PrivateAreaGlyphsVec *Label::createPrivateAreaGlyphs( States::States state )
	{
		std::map<States::States, PrivateAreaGlyphsVec>::iterator itor =
			m_privateAreaGlyphs.find( state );
		if( itor != m_privateAreaGlyphs.end() )
			return &itor->second;

		if( !m_rasterPrivateArea )
		{
			ShaperManager *shaperManager = m_manager->getShaperManager();
			m_rasterPrivateArea = m_manager->createWidget<LabelBmp>( this );
			m_rasterPrivateArea->m_rawMode = true;
			m_rasterPrivateArea->setFont( shaperManager->getDefaultBmpFontForRasterIdx() );
			m_rasterPrivateArea->setSize( m_size );
		}

		auto insertedIt = m_privateAreaGlyphs.insert( { state, PrivateAreaGlyphsVec() } );
		return &insertedIt.first->second;
	}
	//-------------------------------------------------------------------------
	Label::PrivateAreaGlyphsVec *Label::getPrivateAreaGlyphs( States::States state )
	{
		std::map<States::States, PrivateAreaGlyphsVec>::iterator itor =
			m_privateAreaGlyphs.find( state );
		if( itor != m_privateAreaGlyphs.end() )
			return &itor->second;
		return nullptr;
	}
	//-------------------------------------------------------------------------
	void Label::populateRasterPrivateArea()
	{
		PrivateAreaGlyphsVec *privateAreaGlyphs = getPrivateAreaGlyphs( m_currentState );

		if( !privateAreaGlyphs )
		{
			if( m_rasterPrivateArea )
				m_rasterPrivateArea->setHidden( true );
		}
		else
		{
			m_rasterPrivateArea->setHidden( privateAreaGlyphs->empty() );
			m_rasterPrivateArea->m_shapes.clear();

			const States::States currentState = m_currentState;

			ShaperManager *shaperManager = m_manager->getShaperManager();
			const BmpFont *font = shaperManager->getDefaultBmpFontForRaster();

			PrivateAreaGlyphsVec::const_iterator itor = privateAreaGlyphs->begin();
			PrivateAreaGlyphsVec::const_iterator endt = privateAreaGlyphs->end();

			while( itor != endt )
			{
				const ShapedGlyph &shapedGlyph = m_shapes[currentState][*itor];
				Ogre::Vector2 topLeft, bottomRight;
				getCorners( shapedGlyph, topLeft, bottomRight );

				font->renderCodepoint( shapedGlyph.glyph->codepoint, m_rasterPrivateArea->m_shapes );
				BmpGlyph &bmpGlyph = m_rasterPrivateArea->m_shapes.back();

				bmpGlyph.width = shapedGlyph.glyph->width;
				bmpGlyph.height = shapedGlyph.glyph->height;

				// Because the BMP glyph may have different dimensions than the surrogate glyph
				// from freetype, we ensure it is centered.
				topLeft =
					( ( topLeft + bottomRight ) - Ogre::Vector2( bmpGlyph.width, bmpGlyph.height ) ) *
					0.5f;
				bmpGlyph.xoffset = static_cast<uint16_t>( topLeft.x );
				bmpGlyph.yoffset = static_cast<uint16_t>( topLeft.y );
				++itor;
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::setTextHorizAlignment( TextHorizAlignment::TextHorizAlignment horizAlignment )
	{
		if( m_horizAlignment != horizAlignment )
		{
			m_horizAlignment = horizAlignment;
			for( size_t i = 0; i < States::NumStates; ++i )
				flagDirty( static_cast<States::States>( i ) );
		}
	}
	//-------------------------------------------------------------------------
	void Label::setTextVertAlignment( TextVertAlignment::TextVertAlignment vertAlignment )
	{
		if( m_vertAlignment != vertAlignment )
		{
			m_vertAlignment = vertAlignment;
			for( size_t i = 0; i < States::NumStates; ++i )
				flagDirty( static_cast<States::States>( i ) );
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
			for( size_t i = 0; i < States::NumStates; ++i )
				flagDirty( static_cast<States::States>( i ) );
		}
	}
	//-------------------------------------------------------------------------
	VertReadingDir::VertReadingDir Label::getVertReadingDir() const { return m_vertReadingDir; }
	//-------------------------------------------------------------------------
	void Label::setShadowOutline( bool enable, Ogre::ColourValue shadowColour,
								  const Ogre::Vector2 &shadowDisplace )
	{
		m_shadowOutline = enable;
		m_shadowColour = shadowColour;
		m_shadowDisplace = shadowDisplace;
	}
	//-------------------------------------------------------------------------
	void Label::setDefaultFontSize( FontSize defaultFontSize )
	{
		if( m_defaultFontSize != defaultFontSize )
		{
			m_defaultFontSize = defaultFontSize;

			for( size_t i = 0; i < States::NumStates; ++i )
			{
				if( m_richText[i].size() == 1u )
				{
					m_richText[i].clear();
					flagDirty( static_cast<States::States>( i ) );
				}
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::setDefaultFont( uint16_t defaultFont )
	{
		if( m_defaultFont != defaultFont )
		{
			m_defaultFont = defaultFont;
			for( size_t i = 0; i < States::NumStates; ++i )
			{
				if( m_richText[i].size() == 1u )
				{
					m_richText[i].clear();
					flagDirty( static_cast<States::States>( i ) );
				}
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::setLineHeightScale( float lineHeightScale, float lastLineHeightScale )
	{
		m_lineHeightScale = lineHeightScale;
		m_lastLineHeightScale = lastLineHeightScale;
	}
	//-------------------------------------------------------------------------
	void Label::setTextColour( const Ogre::ColourValue &colour, size_t richTextTextIdx,
							   States::States forState )
	{
		m_defaultColour = colour;
		if( forState == States::NumStates )
		{
			for( size_t i = 0; i < States::NumStates; ++i )
				setTextColour( colour, richTextTextIdx, static_cast<States::States>( i ) );
		}
		else
		{
			if( richTextTextIdx == (size_t)-1 )
			{
				RichTextVec::iterator itor = m_richText[forState].begin();
				RichTextVec::iterator endt = m_richText[forState].end();

				while( itor != endt )
				{
					itor->rgba32 = m_defaultColour.getAsABGR();
					++itor;
				}
			}
			else if( richTextTextIdx < m_richText[forState].size() )
			{
				m_richText[forState][richTextTextIdx].rgba32 = m_defaultColour.getAsABGR();
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::validateRichText( States::States state )
	{
		const size_t textSize = m_text[state].size();
		if( m_richText[state].empty() )
		{
			RichText rt = getDefaultRichText();
			rt.length = static_cast<uint32_t>( textSize );
			m_richText[state].push_back( rt );

			m_usesBackground = false;
		}
		else
		{
			bool invalidRtDetected = false;
			RichTextVec::iterator itor = m_richText[state].begin();
			RichTextVec::iterator end = m_richText[state].end();

			while( itor != end )
			{
				if( itor->offset > textSize )
				{
					itor->offset = static_cast<uint32_t>( textSize );
					invalidRtDetected = true;
				}
				if( itor->offset + itor->length > textSize )
				{
					itor->length = static_cast<uint32_t>( textSize - itor->offset );
					invalidRtDetected = true;
				}

				m_usesBackground |= !itor->noBackground;

				++itor;
			}

			if( invalidRtDetected )
			{
				LogListener *log = m_manager->getLogListener();
				char tmpBuffer[512];
				Ogre::LwString errorMsg(
					Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

				errorMsg.clear();
				errorMsg.a(
					"[Label::validateRichText] Rich Edit goes out of bounds. "
					"We've corrected the situation. Text may not be drawn as expected."
					" String: ",
					m_text[state].c_str() );
				log->log( errorMsg.c_str(), LogSeverity::Warning );
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::updateGlyphs( States::States state, bool bPlaceGlyphs )
	{
		const size_t prevNumGlyphs = m_shapes[state].size();

		ShaperManager *shaperManager = m_manager->getShaperManager();

		{
			ShapedGlyphVec::const_iterator itor = m_shapes[state].begin();
			ShapedGlyphVec::const_iterator end = m_shapes[state].end();

			while( itor != end )
			{
				shaperManager->releaseGlyph( itor->glyph );
				++itor;
			}

			m_shapes[state].clear();
		}

		validateRichText( state );

		// See if we can reuse the results from another state. If so,
		// we just need to copy them and increase the ref counts.
		bool reusableFound = false;
		for( size_t i = 0; i < States::NumStates && !reusableFound; ++i )
		{
			if( i != state && !m_glyphsDirty[i] )
			{
				if( m_text[state] == m_text[i] && m_richText[state] == m_richText[i] )
				{
					m_shapes[state] = m_shapes[i];
					m_glyphsPlaced[state] = m_glyphsPlaced[i];
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
					m_glyphsAligned[state] = m_glyphsAligned[i];
#endif
					m_actualHorizAlignment[state] = m_actualHorizAlignment[i];
					m_actualVertReadingDir[state] = m_actualVertReadingDir[i];

					ShapedGlyphVec::const_iterator itor = m_shapes[state].begin();
					ShapedGlyphVec::const_iterator end = m_shapes[state].end();

					while( itor != end )
					{
						shaperManager->addRefCount( itor->glyph );
						++itor;
					}

					const size_t numRichText = m_richText[state].size();
					for( size_t j = 0; j < numRichText; ++j )
					{
						m_richText[state][j].glyphStart = m_richText[i][j].glyphStart;
						m_richText[state][j].glyphEnd = m_richText[i][j].glyphEnd;
					}

					PrivateAreaGlyphsVec *privateAreaGlyphsBase =
						getPrivateAreaGlyphs( static_cast<States::States>( i ) );
					if( privateAreaGlyphsBase )
					{
						PrivateAreaGlyphsVec *privateAreaGlyphs = createPrivateAreaGlyphs( state );
						*privateAreaGlyphs = *privateAreaGlyphsBase;
					}
					else
					{
						PrivateAreaGlyphsVec *privateAreaGlyphs = getPrivateAreaGlyphs( state );
						if( privateAreaGlyphs )
							privateAreaGlyphs->clear();
					}

					if( m_currentState == state && m_glyphsPlaced[state] )
					{
						// placeGlyphs will call populateRasterPrivateArea for us.
						// But otherwise we must do it ourselves.
						populateRasterPrivateArea();
					}

					reusableFound = true;
				}
			}
		}

		if( !reusableFound )
		{
			PrivateAreaGlyphsVec *privateAreaGlyphs = getPrivateAreaGlyphs( state );
			if( privateAreaGlyphs )
				privateAreaGlyphs->clear();

			bool alignmentUnknown = true;
			TextHorizAlignment::TextHorizAlignment actualHorizAlignment = TextHorizAlignment::Mixed;

			RichTextVec::iterator itor = m_richText[state].begin();
			RichTextVec::iterator endt = m_richText[state].end();
			while( itor != endt )
			{
				RichText &richText = *itor;
				richText.glyphStart = static_cast<uint32_t>( m_shapes[state].size() );
				const char *utf8Str = m_text[state].c_str() + richText.offset;
				bool bOutHasPrivateUse = false;
				TextHorizAlignment::TextHorizAlignment actualDir = shaperManager->renderString(
					utf8Str, richText, static_cast<uint32_t>( itor - m_richText[state].begin() ),
					m_vertReadingDir, m_shapes[state], bOutHasPrivateUse );
				richText.glyphEnd = static_cast<uint32_t>( m_shapes[state].size() );

				if( bOutHasPrivateUse && shaperManager->getDefaultBmpFontForRaster() )
				{
					// Collect private area glyphs so we can later populate m_rasterPrivateArea
					privateAreaGlyphs = createPrivateAreaGlyphs( state );

					ShapedGlyphVec::const_iterator it = m_shapes[state].begin() + richText.glyphStart;
					ShapedGlyphVec::const_iterator en = m_shapes[state].begin() + richText.glyphEnd;

					while( it != en )
					{
						if( it->isPrivateArea )
						{
							const uint32_t glyphIdx = uint32_t( it - m_shapes[state].begin() );
							privateAreaGlyphs->push_back( glyphIdx );
						}
						++it;
					}
				}

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

		const size_t currNumGlyphs = m_shapes[state].size();
		if( currNumGlyphs > prevNumGlyphs )
			m_manager->_notifyNumGlyphsIsDirty();
	}
	//-------------------------------------------------------------------------
	void Label::placeGlyphs( States::States state, bool performAlignment )
	{
		const Ogre::Vector2 bottomRight =
			m_size * ( 2.0f * m_manager->getHalfWindowResolution() / m_manager->getCanvasSize() );

		Word nextWord;
		memset( &nextWord, 0, sizeof( Word ) );

		const float vertReadDirSign =
			m_actualVertReadingDir[state] == VertReadingDir::ForceTTB ? -1.0f : 1.0f;

		float largestHeight = findLineMaxHeight( m_shapes[state].begin(), state );
		if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			nextWord.endCaretPos.y += largestHeight;
		else
			nextWord.endCaretPos.x += largestHeight * 0.5f * vertReadDirSign;

		bool multipleWordsInLine = false;

		while( findNextWord( nextWord, state ) )
		{
			if( m_linebreakMode == LinebreakMode::WordWrap )
			{
				if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
				{
					float caretAtEndOfWord =
						nextWord.endCaretPos.x - nextWord.lastAdvance.x + nextWord.lastCharWidth;
					float distBetweenWords = nextWord.endCaretPos.x - nextWord.startCaretPos.x;
					if( caretAtEndOfWord > bottomRight.x &&
						( distBetweenWords <= bottomRight.x || multipleWordsInLine ) &&
						!m_shapes[state][nextWord.offset].isNewline )
					{
						float caretReturn = nextWord.startCaretPos.x;
						float wordLength = nextWord.endCaretPos.x - nextWord.startCaretPos.x;

						// Return to left.
						nextWord.startCaretPos.x -= caretReturn;
						nextWord.endCaretPos.x -= caretReturn;
						// Calculate alignment
						nextWord.startCaretPos.x = 0.0f;
						nextWord.startCaretPos.y += largestHeight;
						nextWord.endCaretPos.x = nextWord.startCaretPos.x + wordLength;
						nextWord.endCaretPos.y = nextWord.startCaretPos.y;
						multipleWordsInLine = false;
					}
				}
				else
				{
					float caretAtEndOfWord =
						nextWord.endCaretPos.y - nextWord.lastAdvance.y + nextWord.lastCharWidth;
					float distBetweenWords = nextWord.endCaretPos.y - nextWord.startCaretPos.y;
					if( caretAtEndOfWord > bottomRight.y &&
						( distBetweenWords <= bottomRight.y || multipleWordsInLine ) &&
						!m_shapes[state][nextWord.offset].isNewline )
					{
						float caretReturn = nextWord.startCaretPos.y;
						float wordLength = nextWord.endCaretPos.y - nextWord.startCaretPos.y;

						// Return to top.
						nextWord.startCaretPos.y -= caretReturn;
						nextWord.endCaretPos.y -= caretReturn;
						// Calculate alignment
						nextWord.startCaretPos.x += largestHeight * vertReadDirSign;
						nextWord.startCaretPos.y = 0.0f;
						nextWord.endCaretPos.x = nextWord.startCaretPos.x;
						nextWord.endCaretPos.y = nextWord.startCaretPos.y + wordLength;
						multipleWordsInLine = false;
					}
				}
			}

			multipleWordsInLine = true;

			Ogre::Vector2 caretPos = nextWord.startCaretPos;

			ShapedGlyphVec::iterator itor = m_shapes[state].begin() + ptrdiff_t( nextWord.offset );
			ShapedGlyphVec::iterator end = itor + ptrdiff_t( nextWord.length );

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
				else /* if( shapedGlyph.isNewline )*/
				{
					shapedGlyph.caretPos = caretPos;
					largestHeight = findLineMaxHeight( itor + 1u, state );

					if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
					{
						// Return to left. Newlines are zero width.
						nextWord.startCaretPos.x = 0.0f;
						nextWord.endCaretPos.x = 0.0f;
						// Calculate alignment
						nextWord.startCaretPos.x = 0.0f;
						nextWord.startCaretPos.y += largestHeight;
					}
					else
					{
						// Return to top. Newlines are zero width.
						nextWord.startCaretPos.y = 0.0f;
						nextWord.endCaretPos.y = 0.0f;
						// Calculate alignment
						nextWord.startCaretPos.x += largestHeight * vertReadDirSign;
						nextWord.startCaretPos.y = 0.0f;
					}
					nextWord.endCaretPos = nextWord.startCaretPos;
					multipleWordsInLine = false;
				}

				++itor;
			}
		}

		m_glyphsPlaced[state] = true;
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = false;
#endif

		if( performAlignment )
			alignGlyphs( state );

		if( state == m_currentState )
			populateRasterPrivateArea();
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
		COLIBRI_ASSERT_MEDIUM( !m_glyphsAligned[state] &&
							   "Calling alignGlyphs twice! updateGlyphs not called?" );
		COLIBRI_ASSERT_LOW( ( m_actualHorizAlignment[state] == TextHorizAlignment::Left ||
							  m_actualHorizAlignment[state] == TextHorizAlignment::Right ||
							  m_actualHorizAlignment[state] == TextHorizAlignment::Center ) &&
							"m_actualHorizAlignment not set! updateGlyphs not called?" );
		COLIBRI_ASSERT_LOW( m_glyphsPlaced[state] && "Did you call placeGlyphs?" );

		if( m_actualHorizAlignment[state] == TextHorizAlignment::Left &&
			m_vertAlignment == TextVertAlignment::Top )
		{
			// Nothing to align
			return;
		}

		float lineWidth = 0;
		float prevCaretY = 0;

		const Ogre::Vector2 widgetBottomRight =
			m_size * ( 2.0f * m_manager->getHalfWindowResolution() / m_manager->getCanvasSize() );

		// To gather width & height
		Ogre::Vector2 maxBottomRight( -std::numeric_limits<float>::max() );
		Ogre::Vector2 minTopLeft( std::numeric_limits<float>::max() );

		ShapedGlyphVec::iterator lineBegin = m_shapes[state].begin();
		ShapedGlyphVec::iterator itor = m_shapes[state].begin();
		ShapedGlyphVec::iterator endt = m_shapes[state].end();

		while( itor != endt )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			Ogre::Vector2 topLeft, bottomRight;
			getAlignmentCorners( shapedGlyph, topLeft, bottomRight );

			if( ( shapedGlyph.isNewline || prevCaretY != shapedGlyph.caretPos.y ) &&
				m_actualHorizAlignment[state] != TextHorizAlignment::Left )
			{
				// Displace horizontally, the line we just were in
				float newLeft = widgetBottomRight.x - lineWidth;
				if( m_actualHorizAlignment[state] == TextHorizAlignment::Center )
					newLeft *= 0.5f;

				while( lineBegin != itor )
				{
					lineBegin->caretPos.x += newLeft;
					++lineBegin;
				}

				prevCaretY = shapedGlyph.caretPos.y;
				lineWidth = 0;
			}

			lineWidth = std::max( lineWidth, bottomRight.x );
			minTopLeft.makeFloor( topLeft );
			maxBottomRight.makeCeil( bottomRight );

			++itor;
		}

		minTopLeft.x = Ogre::Math::Abs( minTopLeft.x );
		minTopLeft.y = Ogre::Math::Abs( minTopLeft.y );
		maxBottomRight.x = Ogre::Math::Abs( maxBottomRight.x );
		maxBottomRight.y = Ogre::Math::Abs( maxBottomRight.y );

		if( m_actualHorizAlignment[state] != TextHorizAlignment::Left )
		{
			// Displace horizontally, last line
			float newLeft = widgetBottomRight.x - lineWidth;
			if( m_actualHorizAlignment[state] == TextHorizAlignment::Center )
				newLeft *= 0.5f;

			while( lineBegin != endt )
			{
				lineBegin->caretPos.x += newLeft;
				++lineBegin;
			}
		}

		if( m_vertAlignment != TextVertAlignment::Top && m_vertAlignment != TextVertAlignment::Natural )
		{
			// We need the width & height from m_position to the last glyph. We add "+ abs(minTopLeft)"
			// so that there is equal distance from m_position to the first glyph, and the last glyph to
			// m_position + m_size
			const Ogre::Vector2 maxWidthHeight( maxBottomRight + minTopLeft );

			// Iterate again, to vertically displace the entire string
			float newTop = widgetBottomRight.y - maxWidthHeight.y;
			if( m_vertAlignment == TextVertAlignment::Center )
				newTop *= 0.5f;

			itor = m_shapes[state].begin();
			while( itor != endt )
			{
				itor->caretPos.y += newTop;
				++itor;
			}
		}

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = true;
#endif
	}
	//-------------------------------------------------------------------------
	void Label::alignGlyphsVertReadingDir( States::States state )
	{
		COLIBRI_ASSERT_MEDIUM( !m_glyphsAligned[state] &&
							   "Calling alignGlyphs twice! updateGlyphs not called?" );
		COLIBRI_ASSERT_LOW( ( m_actualHorizAlignment[state] == TextHorizAlignment::Left ||
							  m_actualHorizAlignment[state] == TextHorizAlignment::Right ||
							  m_actualHorizAlignment[state] == TextHorizAlignment::Center ) &&
							"m_actualHorizAlignment not set! updateGlyphs not called?" );
		COLIBRI_ASSERT_LOW( ( m_actualVertReadingDir[state] == VertReadingDir::ForceTTB ||
							  m_actualVertReadingDir[state] == VertReadingDir::ForceTTBLTR ) &&
							"m_actualVertReadingDir not set! updateGlyphs not called?" );
		COLIBRI_ASSERT_LOW( m_glyphsPlaced[state] && "Did you call placeGlyphs?" );

		if( m_vertAlignment == TextVertAlignment::Top &&
			m_actualHorizAlignment[state] == TextHorizAlignment::Left &&
			m_actualVertReadingDir[state] == VertReadingDir::ForceTTBLTR )
		{
			// Nothing to align
			return;
		}

		float lineWidth = 0;
		float prevCaretX = 0;

		const Ogre::Vector2 widgetBottomRight =
			m_size * ( 2.0f * m_manager->getHalfWindowResolution() / m_manager->getCanvasSize() );

		// To gather width & height
		Ogre::Vector2 maxBottomRight( -std::numeric_limits<float>::max() );
		Ogre::Vector2 minTopLeft( std::numeric_limits<float>::max() );

		ShapedGlyphVec::iterator lineBegin = m_shapes[state].begin();
		ShapedGlyphVec::iterator itor = m_shapes[state].begin();
		ShapedGlyphVec::iterator endt = m_shapes[state].end();

		while( itor != endt )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			Ogre::Vector2 topLeft, bottomRight;
			getAlignmentCorners( shapedGlyph, topLeft, bottomRight );

			if( ( shapedGlyph.isNewline || prevCaretX != shapedGlyph.caretPos.x ) &&
				m_vertAlignment > TextVertAlignment::Top )
			{
				// Displace vertically, the line we just were in
				float newTop = widgetBottomRight.y - lineWidth;
				if( m_vertAlignment == TextVertAlignment::Center )
					newTop *= 0.5f;

				while( lineBegin != itor )
				{
					lineBegin->caretPos.y += newTop;
					++lineBegin;
				}

				prevCaretX = shapedGlyph.caretPos.x;
				lineWidth = 0;
			}

			lineWidth = std::max( lineWidth, bottomRight.y );
			minTopLeft.makeFloor( topLeft );
			maxBottomRight.makeCeil( bottomRight );

			++itor;
		}

		if( m_vertAlignment > TextVertAlignment::Top )
		{
			// Displace vertically, last line
			float newTop = widgetBottomRight.y - lineWidth;
			if( m_vertAlignment == TextVertAlignment::Center )
				newTop *= 0.5f;

			while( lineBegin != endt )
			{
				lineBegin->caretPos.y += newTop;
				++lineBegin;
			}
		}

		minTopLeft.x = Ogre::Math::Abs( minTopLeft.x );
		minTopLeft.y = Ogre::Math::Abs( minTopLeft.y );
		maxBottomRight.x = Ogre::Math::Abs( maxBottomRight.x );
		maxBottomRight.y = Ogre::Math::Abs( maxBottomRight.y );

		if( m_actualHorizAlignment[state] != TextHorizAlignment::Left ||
			m_actualVertReadingDir[state] == VertReadingDir::ForceTTB )
		{
			// We need the width & height from m_position to the last glyph. We add "+ abs(minTopLeft)"
			// so that there is equal distance from m_position to the first glyph, and the last glyph to
			// m_position + m_size
			const Ogre::Vector2 maxWidthHeight( maxBottomRight + minTopLeft );

			// Iterate again, to horizontally displace the entire string
			float newLeft = widgetBottomRight.x - maxWidthHeight.x;
			if( m_actualVertReadingDir[state] == VertReadingDir::ForceTTB )
			{
				switch( m_actualHorizAlignment[state] )
				{
				case TextHorizAlignment::Left:
					newLeft = maxWidthHeight.x;
					break;
				case TextHorizAlignment::Center:
					newLeft = ( widgetBottomRight.x + maxWidthHeight.x ) * 0.5f;
					break;
				case TextHorizAlignment::Natural:
				case TextHorizAlignment::Right:
					newLeft = widgetBottomRight.x;
					break;
				}
			}
			else
			{
				switch( m_actualHorizAlignment[state] )
				{
				case TextHorizAlignment::Natural:
				case TextHorizAlignment::Left:
					newLeft = 0.0f;
					break;
				case TextHorizAlignment::Center:
					newLeft = ( widgetBottomRight.x - maxWidthHeight.x ) * 0.5f;
					break;
				case TextHorizAlignment::Right:
					newLeft = widgetBottomRight.x - maxWidthHeight.x;
					break;
				}
			}

			itor = m_shapes[state].begin();
			while( itor != endt )
			{
				itor->caretPos.x += newLeft;
				++itor;
			}
		}

#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = true;
#endif
	}
	//-------------------------------------------------------------------------
	inline void Label::addQuad( GlyphVertex *RESTRICT_ALIAS vertexBuffer, Ogre::Vector2 topLeft,
								Ogre::Vector2 bottomRight, uint16_t glyphWidth, uint16_t glyphHeight,
								uint32_t rgbaColour, Ogre::Vector2 parentDerivedTL,
								Ogre::Vector2 parentDerivedBR, Ogre::Vector2 invSize, uint32_t offset,
								float canvasAspectRatio, float invCanvasAspectRatio,
								Matrix2x3 derivedRot )
	{
		TODO_this_is_a_workaround_neg_y;
		Ogre::Vector2 tmp2d;

#define COLIBRI_ADD_VERTEX( _x, _y, _u, _v, clipDistanceTop, clipDistanceLeft, clipDistanceRight, \
							clipDistanceBottom ) \
	tmp2d = Widget::mul( derivedRot, _x, _y * invCanvasAspectRatio ); \
	tmp2d.y *= canvasAspectRatio; \
	vertexBuffer->x = tmp2d.x; \
	vertexBuffer->y = -tmp2d.y; \
	vertexBuffer->width = glyphWidth; \
	vertexBuffer->height = glyphHeight; \
	vertexBuffer->offset = offset; \
	vertexBuffer->rgbaColour = rgbaColour; \
	vertexBuffer->clipDistance[Borders::Top] = clipDistanceTop; \
	vertexBuffer->clipDistance[Borders::Left] = clipDistanceLeft; \
	vertexBuffer->clipDistance[Borders::Right] = clipDistanceRight; \
	vertexBuffer->clipDistance[Borders::Bottom] = clipDistanceBottom; \
	++vertexBuffer

		COLIBRI_ADD_VERTEX( topLeft.x, topLeft.y, 0u, 0u, ( topLeft.y - parentDerivedTL.y ) * invSize.y,
							( topLeft.x - parentDerivedTL.x ) * invSize.x,
							( parentDerivedBR.x - topLeft.x ) * invSize.x,
							( parentDerivedBR.y - topLeft.y ) * invSize.y );

		COLIBRI_ADD_VERTEX(
			topLeft.x, bottomRight.y, 0u, glyphHeight, ( bottomRight.y - parentDerivedTL.y ) * invSize.y,
			( topLeft.x - parentDerivedTL.x ) * invSize.x, ( parentDerivedBR.x - topLeft.x ) * invSize.x,
			( parentDerivedBR.y - bottomRight.y ) * invSize.y );

		COLIBRI_ADD_VERTEX( bottomRight.x, bottomRight.y, glyphWidth, glyphHeight,
							( bottomRight.y - parentDerivedTL.y ) * invSize.y,
							( bottomRight.x - parentDerivedTL.x ) * invSize.x,
							( parentDerivedBR.x - bottomRight.x ) * invSize.x,
							( parentDerivedBR.y - bottomRight.y ) * invSize.y );

		COLIBRI_ADD_VERTEX( bottomRight.x, bottomRight.y, glyphWidth, glyphHeight,
							( bottomRight.y - parentDerivedTL.y ) * invSize.y,
							( bottomRight.x - parentDerivedTL.x ) * invSize.x,
							( parentDerivedBR.x - bottomRight.x ) * invSize.x,
							( parentDerivedBR.y - bottomRight.y ) * invSize.y );

		COLIBRI_ADD_VERTEX( bottomRight.x, topLeft.y, glyphWidth, 0u,
							( topLeft.y - parentDerivedTL.y ) * invSize.y,
							( bottomRight.x - parentDerivedTL.x ) * invSize.x,
							( parentDerivedBR.x - bottomRight.x ) * invSize.x,
							( parentDerivedBR.y - topLeft.y ) * invSize.y );

		COLIBRI_ADD_VERTEX( topLeft.x, topLeft.y, 0u, 0u, ( topLeft.y - parentDerivedTL.y ) * invSize.y,
							( topLeft.x - parentDerivedTL.x ) * invSize.x,
							( parentDerivedBR.x - topLeft.x ) * invSize.x,
							( parentDerivedBR.y - topLeft.y ) * invSize.y );

#undef COLIBRI_ADD_VERTEX
	}
	//-------------------------------------------------------------------------
	bool Label::findNextWord( Word &inOutWord, States::States state ) const
	{
		COLIBRI_ASSERT_LOW( inOutWord.offset <= m_shapes[state].size() &&
							inOutWord.offset + inOutWord.length <= m_shapes[state].size() );

		Word word = inOutWord;

		if( word.offset == m_shapes[state].size() ||
			word.offset + word.length == m_shapes[state].size() )
		{
			word.length = 0;
			word.lastAdvance = 0;
			word.lastCharWidth = 0;
			// word.endCaretPos		= Ogre::Vector2::ZERO;
			return false;
		}

		word.offset = word.offset + word.length;

		ShapedGlyphVec::const_iterator itor = m_shapes[state].begin() + ptrdiff_t( word.offset );
		ShapedGlyphVec::const_iterator endt = m_shapes[state].end();

		ShapedGlyph firstGlyph = *itor;
		word.startCaretPos = word.endCaretPos;
		word.endCaretPos += firstGlyph.advance;
		word.lastAdvance = firstGlyph.advance;
		if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			word.lastCharWidth = firstGlyph.glyph->width;
		else
			word.lastCharWidth = firstGlyph.glyph->height;
		const bool isRtl = firstGlyph.isRtl;
		++itor;

		if( !firstGlyph.isNewline && !firstGlyph.isWordBreaker )
		{
			while( itor != endt && !itor->isNewline && !itor->isWordBreaker && itor->isRtl == isRtl )
			{
				const ShapedGlyph &shapedGlyph = *itor;
				word.endCaretPos += shapedGlyph.advance;
				word.lastAdvance = shapedGlyph.advance;
				if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
					word.lastCharWidth = shapedGlyph.glyph->width;
				else
					word.lastCharWidth = shapedGlyph.glyph->height;
				++itor;
			}
		}
		else if( firstGlyph.isTab )
		{
			if( m_actualVertReadingDir[state] == VertReadingDir::Disabled )
			{
				word.endCaretPos.x = ceilf( ( word.startCaretPos.x + firstGlyph.advance.x * 0.25f ) /
											( firstGlyph.advance.x * 2.0f ) ) *
									 ( firstGlyph.advance.x * 2.0f );
			}
			else
			{
				word.endCaretPos.y = ceilf( ( word.startCaretPos.y + firstGlyph.advance.y * 0.25f ) /
											( firstGlyph.advance.y * 4.0f ) ) *
									 ( firstGlyph.advance.y * 4.0f );
			}
		}

		word.length =
			static_cast<size_t>( itor - ( m_shapes[state].begin() + ptrdiff_t( word.offset ) ) );

		inOutWord = word;

		return word.length != 0;
	}
	//-------------------------------------------------------------------------
	float Label::findLineMaxHeight( ShapedGlyphVec::const_iterator start, States::States state ) const
	{
		COLIBRI_ASSERT_LOW( start >= m_shapes[state].begin() && start <= m_shapes[state].end() );

		float largestHeight = 0;

		ShapedGlyphVec::const_iterator itor = start;
		ShapedGlyphVec::const_iterator endt = m_shapes[state].end();
		while( itor != endt && !itor->isNewline )
		{
			largestHeight = std::max( itor->glyph->newlineSize, largestHeight );
			++itor;
		}

		// The newline itself has its own height, make sure it's considered
		if( itor != endt )
			largestHeight = std::max( itor->glyph->newlineSize, largestHeight ) * m_lineHeightScale;
		else
			largestHeight *= m_lastLineHeightScale;

		return largestHeight;
	}
	//-------------------------------------------------------------------------
	GlyphVertex *Label::fillBackground( GlyphVertex *RESTRICT_ALIAS textVertBuffer,
										const Ogre::Vector2 halfWindowRes,
										const Ogre::Vector2 invWindowRes,
										const Ogre::Vector2 parentDerivedTL,
										const Ogre::Vector2 parentDerivedBR, const bool isHorizontal )
	{
		const Ogre::Vector2 invSize = 1.0f / ( parentDerivedBR - parentDerivedTL );

		// Snap position to pixels
		Ogre::Vector2 derivedTopLeft = m_derivedTopLeft;
		derivedTopLeft = ( derivedTopLeft + 1.0f ) * halfWindowRes;
		derivedTopLeft.x = roundf( derivedTopLeft.x );
		derivedTopLeft.y = roundf( derivedTopLeft.y );
		derivedTopLeft = derivedTopLeft * invWindowRes - 1.0f;

		const Matrix2x3 derivedRot = m_derivedOrientation;
		const float canvasAr = m_manager->getCanvasAspectRatio();
		const float invCanvasAr = m_manager->getCanvasInvAspectRatio();

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
				float mostTop = std::numeric_limits<float>::max();
				float mostLeft = std::numeric_limits<float>::max();
				float mostRight = -std::numeric_limits<float>::max();
				float mostBottom = -std::numeric_limits<float>::max();

				ShapedGlyphVec::const_iterator itor =
					m_shapes[m_currentState].begin() + itRichText->glyphStart;
				ShapedGlyphVec::const_iterator end =
					m_shapes[m_currentState].begin() + itRichText->glyphEnd;

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
											 ( prevCaretY != shapedGlyph.caretPos.y && isHorizontal ) ||
											 ( prevCaretY != shapedGlyph.caretPos.x && !isHorizontal );

					if( !shapedGlyph.isNewline && !changesLine )
					{
						lineHeight = std::max( lineHeight, shapedGlyph.glyph->newlineSize );
						mostTop = std::min( mostTop, shapedGlyph.caretPos.y );
						mostBottom = std::max( mostBottom, shapedGlyph.caretPos.y );
						if( isHorizontal )
						{
							mostLeft =
								std::min( mostLeft, shapedGlyph.caretPos.x + shapedGlyph.offset.x +
														shapedGlyph.glyph->bearingX );
							mostRight = std::max(
								mostRight, shapedGlyph.caretPos.x + shapedGlyph.offset.x +
											   shapedGlyph.glyph->bearingX + shapedGlyph.glyph->width );
						}
						else
						{
							mostLeft = std::min( mostLeft, shapedGlyph.caretPos.x - lineHeight * 0.5f );
							mostRight =
								std::max( mostRight, shapedGlyph.caretPos.x + lineHeight * 0.5f );
						}
					}

					if( itor + 1u == end || changesLine )
					{
						const float regionUp = isHorizontal ? shapedGlyph.glyph->regionUp : 0.0f;

						// New line found. Render the background and reset the counters
						Ogre::Vector2 topLeft =
							Ogre::Vector2( mostLeft, mostTop - lineHeight * regionUp );
						Ogre::Vector2 bottomRight =
							Ogre::Vector2( mostRight, mostBottom + lineHeight * ( 1.0f - regionUp ) );

						const Ogre::Vector2 glyphSize = bottomRight - topLeft;

						// Snap each glyph to pixels too
						topLeft.x = roundf( topLeft.x );
						topLeft.y = roundf( topLeft.y );
						bottomRight = topLeft + glyphSize;

						topLeft = derivedTopLeft + topLeft * invWindowRes;
						bottomRight = derivedTopLeft + bottomRight * invWindowRes;

						addQuad( textVertBuffer,                                               //
								 topLeft - backgroundDisplacement,                             //
								 bottomRight + backgroundDisplacement,                         //
								 1, 1,                                                         //
								 backgroundColour, parentDerivedTL, parentDerivedBR, invSize,  //
								 0,                                                            //
								 canvasAr, invCanvasAr, derivedRot );
						textVertBuffer += 6u;
						m_numVertices += 6u;

						Ogre::Vector2 nextCaret = shapedGlyph.caretPos;
						if( shapedGlyph.isNewline && itor + 1u != end )
							nextCaret = ( itor + 1u )->caretPos;

						if( !isHorizontal )
							prevCaretY = nextCaret.x;
						else
							prevCaretY = nextCaret.y;
						lineHeight = 0;
						mostTop = std::numeric_limits<float>::max();
						mostLeft = std::numeric_limits<float>::max();
						mostRight = -std::numeric_limits<float>::max();
						mostBottom = -std::numeric_limits<float>::max();

						// Step backwards and reprocess this glyph again,
						// as part of the new line and not the current one.
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
	void Label::_fillBuffersAndCommands( UiVertex **RESTRICT_ALIAS vertexBuffer,
										 GlyphVertex **RESTRICT_ALIAS _textVertBuffer,
										 const Ogre::Vector2 &parentPos,
										 const Ogre::Vector2 &parentCurrentScrollPos,
										 const Matrix2x3 &parentRot )
	{
		GlyphVertex *RESTRICT_ALIAS textVertBuffer = *_textVertBuffer;

		updateDerivedTransform( parentPos, parentRot );

		m_culled = true;

		m_numVertices = 0;
		if( !m_parent->intersectsChild( this, parentCurrentScrollPos ) || m_hidden )
			return;

		m_culled = false;

		if( !m_visualsEnabled )
			return;

		m_currVertexBufferOffset =
			static_cast<uint32_t>( textVertBuffer - m_manager->_getTextVertexBufferBase() );

		const uint32_t shadowColour = ( m_shadowColour * m_colour ).getAsABGR();

		const Ogre::Vector2 halfWindowRes = m_manager->getHalfWindowResolution();
		const Ogre::Vector2 invWindowRes = m_manager->getInvWindowResolution2x();

		const Ogre::Vector2 shadowDisplacement = invWindowRes * m_shadowDisplace;

		Ogre::Vector2 invCanvasSize2x = m_manager->getInvCanvasSize2x();
		Ogre::Vector2 parentDerivedTL =
			m_parent->m_derivedTopLeft + m_parent->m_clipBorderTL * invCanvasSize2x;
		Ogre::Vector2 parentDerivedBR =
			m_parent->m_derivedBottomRight - m_parent->m_clipBorderBR * invCanvasSize2x;
		parentDerivedTL.makeCeil( m_parent->m_accumMinClipTL );
		parentDerivedBR.makeFloor( m_parent->m_accumMaxClipBR );
		m_accumMinClipTL = parentDerivedTL;
		m_accumMaxClipBR = parentDerivedBR;
		if( m_clipTextToWidget )
		{
			parentDerivedTL.makeCeil( this->m_derivedTopLeft );
			parentDerivedBR.makeFloor( this->m_derivedBottomRight );
		}

		const Ogre::Vector2 invSize = 1.0f / ( parentDerivedBR - parentDerivedTL );

		if( m_usesBackground )
		{
			const bool isHoriz = m_actualVertReadingDir[m_currentState] == VertReadingDir::Disabled;
			textVertBuffer = fillBackground( textVertBuffer, halfWindowRes, invWindowRes,
											 parentDerivedTL, parentDerivedBR, isHoriz );
		}

		// Snap position to pixels
		Ogre::Vector2 derivedTopLeft = m_derivedTopLeft;
		derivedTopLeft = ( derivedTopLeft + 1.0f ) * halfWindowRes;
		derivedTopLeft.x = roundf( derivedTopLeft.x );
		derivedTopLeft.y = roundf( derivedTopLeft.y );
		derivedTopLeft = derivedTopLeft * invWindowRes - 1.0f;

		const Matrix2x3 derivedRot = m_derivedOrientation;
		const float canvasAr = m_manager->getCanvasAspectRatio();
		const float invCanvasAr = m_manager->getCanvasInvAspectRatio();

		const uint8_t colourRgba8[4] = { static_cast<uint8_t>( m_colour.r * 255.0f ),
										 static_cast<uint8_t>( m_colour.g * 255.0f ),
										 static_cast<uint8_t>( m_colour.b * 255.0f ),
										 static_cast<uint8_t>( m_colour.a * 255.0f ) };

		ShapedGlyphVec::const_iterator itor = m_shapes[m_currentState].begin();
		ShapedGlyphVec::const_iterator endt = m_shapes[m_currentState].end();

		while( itor != endt )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			if( !shapedGlyph.isNewline && !shapedGlyph.isTab && !shapedGlyph.isPrivateArea )
			{
				Ogre::Vector2 topLeft, bottomRight;
				getCorners( shapedGlyph, topLeft, bottomRight );

				const Ogre::Vector2 glyphSize = bottomRight - topLeft;

				// Snap each glyph to pixels too
				topLeft.x = roundf( topLeft.x );
				topLeft.y = roundf( topLeft.y );
				bottomRight = topLeft + glyphSize;

				topLeft = derivedTopLeft + topLeft * invWindowRes;
				bottomRight = derivedTopLeft + bottomRight * invWindowRes;

				if( m_shadowOutline )
				{
					addQuad( textVertBuffer,                                           //
							 topLeft + shadowDisplacement,                             //
							 bottomRight + shadowDisplacement,                         //
							 shapedGlyph.glyph->width, shapedGlyph.glyph->height,      //
							 shadowColour, parentDerivedTL, parentDerivedBR, invSize,  //
							 shapedGlyph.glyph->offsetStart,                           //
							 canvasAr, invCanvasAr, derivedRot );
					textVertBuffer += 6u;
					m_numVertices += 6u;
				}

				const RichText &richText = m_richText[m_currentState][shapedGlyph.richTextIdx];

				const uint32_t oldRgba32 = richText.rgba32;
				uint32_t newRgba32 = 0u;

				newRgba32 |= ( ( oldRgba32 & 0xFFu ) * colourRgba8[0] ) / 255u;
				newRgba32 |= ( ( ( ( oldRgba32 >> 8u ) & 0xFFu ) * colourRgba8[1] ) / 255u ) << 8u;
				newRgba32 |= ( ( ( ( oldRgba32 >> 16u ) & 0xFFu ) * colourRgba8[2] ) / 255u ) << 16u;
				newRgba32 |= ( ( ( ( oldRgba32 >> 24u ) & 0xFFu ) * colourRgba8[3] ) / 255u ) << 24u;

				addQuad( textVertBuffer, topLeft, bottomRight,                  //
						 shapedGlyph.glyph->width, shapedGlyph.glyph->height,   //
						 newRgba32, parentDerivedTL, parentDerivedBR, invSize,  //
						 shapedGlyph.glyph->offsetStart,                        //
						 canvasAr, invCanvasAr, derivedRot );
				textVertBuffer += 6u;

				m_numVertices += 6u;
			}

			++itor;
		}

		*_textVertBuffer = textVertBuffer;

		const Ogre::Vector2 outerTopLeft = this->m_derivedTopLeft;
		const Matrix2x3 &finalRot = this->m_derivedOrientation;
		const Ogre::Vector2 outerTopLeftWithClipping = outerTopLeft + m_clipBorderTL * invCanvasSize2x;

		WidgetVec::const_iterator itChild = m_children.begin();
		WidgetVec::const_iterator enChild = m_children.end();

		while( itChild != enChild )
		{
			( *itChild )
				->_fillBuffersAndCommands( vertexBuffer, _textVertBuffer, outerTopLeftWithClipping,
										   Ogre::Vector2::ZERO, finalRot );
			++itChild;
		}
	}
	//-------------------------------------------------------------------------
	void Label::_updateDirtyGlyphs()
	{
		for( size_t i = 0; i < States::NumStates; ++i )
		{
			if( m_glyphsDirty[i] )
				updateGlyphs( static_cast<States::States>( i ) );

			if( !m_glyphsPlaced[i] )
				placeGlyphs( static_cast<States::States>( i ) );
		}
	}
	//-------------------------------------------------------------------------
	bool Label::isAnyStateDirty() const
	{
		bool retVal = false;
		for( size_t i = 0; i < States::NumStates; ++i )
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
#if COLIBRIGUI_DEBUG >= COLIBRIGUI_DEBUG_MEDIUM
		m_glyphsAligned[state] = false;
#endif
		m_usesBackground = false;
	}
	//-------------------------------------------------------------------------
	size_t Label::getMaxNumGlyphs() const
	{
		size_t retVal = 0;
		for( size_t i = 0; i < States::NumStates; ++i )
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
			for( size_t i = 0; i < States::NumStates; ++i )
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
	const std::string &Label::getText( States::States state )
	{
		if( state == States::NumStates )
			state = m_currentState;
		return m_text[state];
	}
	//-------------------------------------------------------------------------
	void Label::setRichText( RichTextVec &richText, bool bSwap, States::States forState )
	{
		if( forState == States::NumStates )
		{
			if( bSwap )
				m_richText[0].swap( richText );
			else
				m_richText[0] = richText;

			flagDirty( static_cast<States::States>( 0u ) );

			for( size_t i = 1u; i < States::NumStates; ++i )
			{
				m_richText[i] = m_richText[0];
				flagDirty( static_cast<States::States>( i ) );
			}
		}
		else
		{
			if( bSwap )
				m_richText[forState].swap( richText );
			else
				m_richText[forState] = richText;

			flagDirty( forState );
		}
	}
	//-------------------------------------------------------------------------
	RichText Label::getDefaultRichText() const
	{
		RichText rt;
		rt.ptSize = m_defaultFontSize;
		rt.rgba32 = m_defaultColour.getAsABGR();
		rt.noBackground = true;
		rt.backgroundRgba32 = m_defaultBackgroundColour.getAsABGR();
		rt.font = m_defaultFont;
		rt.offset = 0;
		rt.length = 0u;
		rt.readingDir = HorizReadingDir::Default;
		rt.glyphStart = rt.glyphEnd = 0;

		return rt;
	}
	//-------------------------------------------------------------------------
	size_t Label::getGlyphCount( States::States state ) const
	{
		if( state == States::NumStates )
			state = m_currentState;

		return m_shapes[m_currentState].size();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Label::getCaretTopLeft( size_t glyphIdx, FontSize &ptSize, uint16_t &outFontIdx ) const
	{
		COLIBRI_ASSERT_MEDIUM( !isAnyStateDirty() );

		Ogre::Vector2 localTopLeft = m_position;

		const Ogre::Vector2 canvasSize = m_manager->getCanvasSize();
		const Ogre::Vector2 invWindowRes = 0.5f * m_manager->getInvWindowResolution2x();

		glyphIdx = std::min( glyphIdx, m_shapes[m_currentState].size() );
		ShapedGlyphVec::const_iterator itor = m_shapes[m_currentState].begin() + ptrdiff_t( glyphIdx );

		if( itor != m_shapes[m_currentState].end() )
		{
			const ShapedGlyph &shapedGlyph = *itor;

			Ogre::Vector2 topLeft;
			topLeft = shapedGlyph.caretPos;
			topLeft.x += shapedGlyph.glyph->bearingX;
			topLeft.y -= shapedGlyph.glyph->newlineSize;
			localTopLeft += topLeft * invWindowRes * canvasSize;

			ptSize = shapedGlyph.glyph->ptSize;
			outFontIdx = shapedGlyph.glyph->font;
		}
		else if( !m_shapes[m_currentState].empty() )
		{
			const ShapedGlyph &shapedGlyph = m_shapes[m_currentState].back();

			Ogre::Vector2 topRight;
			topRight = shapedGlyph.caretPos;
			topRight.x += shapedGlyph.advance.x + 1.0f;
			topRight.y -= shapedGlyph.glyph->newlineSize;
			localTopLeft += topRight * invWindowRes * canvasSize;

			ptSize = shapedGlyph.glyph->ptSize;
			outFontIdx = shapedGlyph.glyph->font;
		}
		else
		{
			ptSize = m_defaultFontSize;
			outFontIdx = m_defaultFont;
		}

		return localTopLeft;
	}
	//-------------------------------------------------------------------------
	size_t Label::advanceGlyphToNextCluster( size_t glyphIdx ) const
	{
		const size_t glyphCount = m_shapes[m_currentState].size();

		if( glyphIdx >= glyphCount )
			return glyphCount;

		const size_t currCluster = m_shapes[m_currentState][glyphIdx].clusterStart;

		size_t nextGlyph = glyphIdx + 1u;

		while( nextGlyph < glyphCount &&
			   currCluster == m_shapes[m_currentState][nextGlyph].clusterStart )
		{
			++nextGlyph;
		}

		return nextGlyph;
	}
	//-------------------------------------------------------------------------
	size_t Label::regressGlyphToPreviousCluster( size_t glyphIdx ) const
	{
		const size_t glyphCount = m_shapes[m_currentState].size();

		if( glyphCount == 0 )
			return glyphCount;

		bool wasOutOfBounds = glyphIdx >= glyphCount;
		if( wasOutOfBounds )
			glyphIdx = glyphCount - 1u;

		// We're counting on the fact that when prevIdx == 0 or glyphIdx == 0;
		// decrementing it will underflow and thus prevIdx < glyphCount is not true.
		const size_t currCluster = m_shapes[m_currentState][glyphIdx].clusterStart;

		size_t prevIdx = glyphIdx - 1u;

		while( prevIdx < glyphCount && currCluster == m_shapes[m_currentState][prevIdx].clusterStart )
		{
			--prevIdx;
		}

		if( wasOutOfBounds )
		{
			// We need to position behind the last letter, not behind the second to last letter
			++prevIdx;
		}

		if( prevIdx >= glyphCount )
			prevIdx = 0;

		return prevIdx;
	}
	//-------------------------------------------------------------------------
	void Label::getGlyphStartUtf16( size_t glyphIdx, size_t &glyphStart, size_t &outLength )
	{
		COLIBRI_ASSERT_MEDIUM( !isAnyStateDirty() );

		if( glyphIdx < m_shapes[m_currentState].size() )
		{
			const ShapedGlyph &shapedGlyph = m_shapes[m_currentState][glyphIdx];
			glyphStart = shapedGlyph.clusterStart;
			outLength = shapedGlyph.clusterLength;
		}
		else
		{
			if( !m_manager->swapRTLControls() )
				glyphStart = m_text[m_currentState].size();
			else
				glyphStart = 0;
			outLength = 0;
		}
	}
	//-------------------------------------------------------------------------
	void Label::sizeToFit( float maxAllowedWidth, TextHorizAlignment::TextHorizAlignment newHorizPos,
						   TextVertAlignment::TextVertAlignment newVertPos, States::States baseState )
	{
		if( baseState == States::NumStates )
			baseState = m_currentState;

		if( m_glyphsDirty[baseState] )
			updateGlyphs( baseState, false );

		if( m_shapes[baseState].empty() )
			return;

		// Replace the glyphs forced to the Top-Left so we can gather the width & height
		const float oldWidth = m_size.x;
		m_size.x = maxAllowedWidth;
		placeGlyphs( baseState, false );
		m_size.x = oldWidth;

		// Gather width & height
		Ogre::Vector2 maxBottomRight( -std::numeric_limits<float>::max() );
		Ogre::Vector2 minTopLeft( std::numeric_limits<float>::max() );

		ShapedGlyphVec::iterator itor = m_shapes[baseState].begin();
		ShapedGlyphVec::iterator end = m_shapes[baseState].end();

		while( itor != end )
		{
			Ogre::Vector2 topLeft, bottomRight;
			getAlignmentCorners( *itor, topLeft, bottomRight );

			minTopLeft.makeFloor( topLeft );
			maxBottomRight.makeCeil( bottomRight );
			++itor;
		}

		minTopLeft.x = Ogre::Math::Abs( minTopLeft.x );
		minTopLeft.y = Ogre::Math::Abs( minTopLeft.y );
		maxBottomRight.x = Ogre::Math::Abs( maxBottomRight.x );
		maxBottomRight.y = Ogre::Math::Abs( maxBottomRight.y );

		// We need the width & height from m_position to the last glyph. We add "+ abs(minTopLeft)" so
		// that there is equal distance from m_position to the first glyph, and the last glyph to
		// m_position + m_size
		const Ogre::Vector2 maxWidthHeight( maxBottomRight + minTopLeft );

		if( maxWidthHeight.x < 0 || maxWidthHeight.y < 0 )
			return;

		// Set new dimensions
		const Ogre::Vector2 canvasSize = m_manager->getCanvasSize();
		const Ogre::Vector2 invWindowRes = 0.5f * m_manager->getInvWindowResolution2x();

		Ogre::Vector2 oldSize = m_size;
		m_size = maxWidthHeight * invWindowRes * canvasSize;
		m_size.x = std::ceil( m_size.x );
		m_size.y = std::ceil( m_size.y );

		// Align the glyphs so horizontal & vertical alignment are respected
		alignGlyphs( baseState );

		// Now reposition the widget based on input newHorizPos & newVertPos
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
			m_position.x = ( m_position.x + oldSize.x - m_size.x ) * 0.5f;
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
			m_position.y = ( m_position.y + oldSize.y - m_size.y ) * 0.5f;
			break;
		case TextVertAlignment::Bottom:
			m_position.y = m_position.y + oldSize.y - m_size.y;
			break;
		}

		m_minSize = m_size;

		if( m_rasterPrivateArea )
		{
			m_rasterPrivateArea->setSize( m_size );
			populateRasterPrivateArea();
		}
	}
	//-------------------------------------------------------------------------
	void Label::setTransformDirty( uint32_t dirtyReason )
	{
		if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) ==
			TransformDirtyScale )
		{
			// Align the glyphs so horizontal & vertical alignment are respected
			if( !m_glyphsDirty[m_currentState] && m_glyphsPlaced[m_currentState] )
				placeGlyphs( m_currentState );

			for( size_t i = 0; i < States::NumStates; ++i )
			{
				if( i != m_currentState )
					m_glyphsPlaced[i] = false;
			}
		}

		if( dirtyReason & TransformDirtyScale )
		{
			if( m_rasterPrivateArea )
				m_rasterPrivateArea->setSize( m_size );
		}

		Renderable::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Label::setState( States::States state, bool smartHighlight )
	{
		const States::States oldState = m_currentState;
		Renderable::setState( state, smartHighlight );

		if( oldState != state )
		{
			// We must replace the glyphs from the new state since it could be
			// non-dirty but its placement out of date (e.g. Label was in Idle,
			// glyphs were placed, then changed to Highlighted state, widget was
			// resized, and now we're going back to Idle with a different size)
			if( !m_glyphsDirty[m_currentState] && !m_glyphsPlaced[m_currentState] )
			{
				placeGlyphs( m_currentState );
			}
			else
			{
				// placeGlyphs will call populateRasterPrivateArea for us.
				// But otherwise we must do it ourselves.
				populateRasterPrivateArea();
			}
		}
	}
	//-------------------------------------------------------------------------
	void Label::_notifyCanvasChanged()
	{
		for( size_t i = 0; i < States::NumStates; ++i )
			flagDirty( static_cast<States::States>( i ) );
		//		_updateDirtyGlyphs();

		Renderable::_notifyCanvasChanged();
	}
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	//-------------------------------------------------------------------------
	bool RichText::operator==( const RichText &other ) const
	{
		return this->ptSize == other.ptSize && this->offset == other.offset &&
			   this->length == other.length && this->readingDir == other.readingDir &&
			   this->font == other.font;
	}
}  // namespace Colibri
