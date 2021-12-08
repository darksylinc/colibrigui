
#include "ColibriGui/ColibriLabelBmp.h"

#include "ColibriGui/Text/ColibriBmpFont.h"
#include "ColibriGui/Text/ColibriShaperManager.h"

#include "ColibriRenderable.inl"

#include "OgreLwString.h"

#include "unicode/unistr.h"

#define TODO_DATABLock

namespace Colibri
{
	LabelBmp::LabelBmp( ColibriManager *manager ) :
		Renderable( manager ),
		m_clipTextToWidget( true ),
		m_shadowOutline( false ),
		m_shadowColour( Ogre::ColourValue::Black ),
		m_shadowDisplace( 1.0f ),
		m_fontSize( m_manager->getDefaultFontSize26d6() ),
		m_font( 0 )
	{
		setVao( m_manager->getVao() );

		m_glyphsDirty = false;
		m_numVertices = 0;

		TODO_DATABLock;
		for( size_t i = 0; i < States::NumStates; ++i )
			m_stateInformation[i].materialName = ColibriManager::c_defaultTextDatablockNames[i];

		ShaperManager *shaperManager = m_manager->getShaperManager();
		Ogre::HlmsDatablock *datablock = shaperManager->getBmpFont( m_font )->getDatablock();
		COLIBRI_ASSERT_MEDIUM(
			datablock &&
			"getBmpFont returned no datablock. Please call setOgre first, and ensure the "
			"ShaperManager (fonts) has already been properly initialized" );
		setDatablock( datablock );
	}
	//-------------------------------------------------------------------------
	void LabelBmp::setShadowOutline( bool enable, Ogre::ColourValue shadowColour,
									 const Ogre::Vector2 &shadowDisplace )
	{
		m_shadowOutline = enable;
		m_shadowColour = shadowColour;
		m_shadowDisplace = shadowDisplace;
	}
	//-------------------------------------------------------------------------
	void LabelBmp::setFontSize( FontSize fontSize ) { m_fontSize = fontSize; }
	//-------------------------------------------------------------------------
	void LabelBmp::setFont( uint16_t font )
	{
		if( m_font != font )
		{
			m_font = font;

			ShaperManager *shaperManager = m_manager->getShaperManager();
			Ogre::HlmsDatablock *datablock = shaperManager->getBmpFont( m_font )->getDatablock();
			COLIBRI_ASSERT_LOW( datablock );
			setDatablock( datablock );

			flagDirty();
		}
	}
	//-------------------------------------------------------------------------
	void LabelBmp::setTextColour( const Ogre::ColourValue &colour ) { m_colour = colour; }
	//-------------------------------------------------------------------------
	void LabelBmp::updateGlyphs()
	{
		const size_t prevNumGlyphs = m_shapes.size();

		ShaperManager *shaperManager = m_manager->getShaperManager();
		BmpFont *font = shaperManager->getBmpFont( m_font );
		font->renderString( m_text[m_currentState], m_shapes );
		m_glyphsDirty = false;

		const size_t currNumGlyphs = m_shapes.size();
		if( currNumGlyphs > prevNumGlyphs )
			m_manager->_notifyNumGlyphsBmpIsDirty();
	}
	//-------------------------------------------------------------------------
	void LabelBmp::_fillBuffersAndCommands( UiVertex **RESTRICT_ALIAS _vertexBuffer,
											GlyphVertex **RESTRICT_ALIAS _textVertBuffer,
											const Ogre::Vector2 &parentPos,
											const Ogre::Vector2 &parentCurrentScrollPos,
											const Matrix2x3 &parentRot )
	{
		UiVertex *RESTRICT_ALIAS vertexBuffer = *_vertexBuffer;

		updateDerivedTransform( parentPos, parentRot );

		m_culled = true;

		m_numVertices = 0;
		if( !m_parent->intersectsChild( this, parentCurrentScrollPos ) || m_hidden )
			return;

		m_culled = false;

		if( !m_visualsEnabled )
			return;

		m_currVertexBufferOffset =
			static_cast<uint32_t>( vertexBuffer - m_manager->_getVertexBufferBase() );

		uint8_t shadowColour[4];
		shadowColour[0] = static_cast<uint8_t>( m_shadowColour.r * 255.0f + 0.5f );
		shadowColour[1] = static_cast<uint8_t>( m_shadowColour.g * 255.0f + 0.5f );
		shadowColour[2] = static_cast<uint8_t>( m_shadowColour.b * 255.0f + 0.5f );
		shadowColour[3] = static_cast<uint8_t>( m_shadowColour.a * 255.0f + 0.5f );

		uint8_t rgbaColour[4];
		rgbaColour[0] = static_cast<uint8_t>( m_colour.r * 255.0f + 0.5f );
		rgbaColour[1] = static_cast<uint8_t>( m_colour.g * 255.0f + 0.5f );
		rgbaColour[2] = static_cast<uint8_t>( m_colour.b * 255.0f + 0.5f );
		rgbaColour[3] = static_cast<uint8_t>( m_colour.a * 255.0f + 0.5f );

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

		// Snap position to pixels
		Ogre::Vector2 derivedTopLeft = m_derivedTopLeft;
		derivedTopLeft = ( derivedTopLeft + 1.0f ) * halfWindowRes;
		derivedTopLeft.x = roundf( derivedTopLeft.x );
		derivedTopLeft.y = roundf( derivedTopLeft.y );
		derivedTopLeft = derivedTopLeft * invWindowRes - 1.0f;

		const Matrix2x3 derivedRot = m_derivedOrientation;
		const float canvasAr = m_manager->getCanvasAspectRatio();
		const float invCanvasAr = m_manager->getCanvasInvAspectRatio();

		ShaperManager *shaperManager = m_manager->getShaperManager();
		const BmpFont *bmpFont = shaperManager->getBmpFont( m_font );

		const float fontScale = m_fontSize.asFloat() / bmpFont->getBakedFontSize().asFloat();
		Ogre::Vector2 currentTopLeft( Ogre::Vector2::ZERO );

		const Ogre::Vector4 texInvResolution( bmpFont->getInvResolution() );

		BmpGlyphVec::const_iterator itor = m_shapes.begin();
		BmpGlyphVec::const_iterator endt = m_shapes.end();

		while( itor != endt )
		{
			const BmpGlyph &bmpGlyph = *itor;

			if( !bmpGlyph.isNewline && !bmpGlyph.isTab )
			{
				Ogre::Vector2 topLeft = currentTopLeft + Ogre::Vector2( bmpGlyph.bmpChar->xoffset,
																		bmpGlyph.bmpChar->yoffset );
				Ogre::Vector2 bottomRight =
					topLeft + Ogre::Vector2( bmpGlyph.bmpChar->width, bmpGlyph.bmpChar->height );

				topLeft *= fontScale;
				bottomRight *= fontScale;

				const Ogre::Vector2 glyphSize = bottomRight - topLeft;

				// Snap each glyph to pixels too
				topLeft.x = roundf( topLeft.x );
				topLeft.y = roundf( topLeft.y );
				bottomRight = topLeft + glyphSize;

				topLeft = derivedTopLeft + topLeft * invWindowRes;
				bottomRight = derivedTopLeft + bottomRight * invWindowRes;

				if( m_shadowOutline )
				{
					addQuad( vertexBuffer,                      //
							 topLeft + shadowDisplacement,      //
							 bottomRight + shadowDisplacement,  //
							 ( Ogre::Vector4( bmpGlyph.bmpChar->x, bmpGlyph.bmpChar->y,
											  bmpGlyph.bmpChar->x + bmpGlyph.bmpChar->width,
											  bmpGlyph.bmpChar->y + bmpGlyph.bmpChar->height ) +
							   0.5f ) *
								 texInvResolution,
							 shadowColour, parentDerivedTL, parentDerivedBR, invSize,  //
							 canvasAr, invCanvasAr, derivedRot );
					vertexBuffer += 6u;
					m_numVertices += 6u;
				}

				addQuad( vertexBuffer, topLeft, bottomRight,  //
						 ( Ogre::Vector4( bmpGlyph.bmpChar->x, bmpGlyph.bmpChar->y,
										  bmpGlyph.bmpChar->x + bmpGlyph.bmpChar->width,
										  bmpGlyph.bmpChar->y + bmpGlyph.bmpChar->height ) +
						   0.5f ) *
							 texInvResolution,
						 rgbaColour, parentDerivedTL, parentDerivedBR, invSize,  //
						 canvasAr, invCanvasAr, derivedRot );
				vertexBuffer += 6u;

				m_numVertices += 6u;
			}

			if( itor->isNewline )
			{
				currentTopLeft.x = 0.0f;
				currentTopLeft.y += itor->bmpChar->yoffset + itor->bmpChar->height;
			}
			else if( itor->isTab )
			{
				currentTopLeft.x = ceilf( ( currentTopLeft.x + itor->bmpChar->xadvance * 0.25f ) /
										  ( itor->bmpChar->xadvance * 2.0f ) ) *
								   ( itor->bmpChar->xadvance * 2.0f );
			}
			else
			{
				currentTopLeft.x += itor->bmpChar->xadvance;
			}

			++itor;
		}

		*_vertexBuffer = vertexBuffer;

		const Ogre::Vector2 outerTopLeft = this->m_derivedTopLeft;
		const Matrix2x3 &finalRot = this->m_derivedOrientation;
		const Ogre::Vector2 outerTopLeftWithClipping = outerTopLeft + m_clipBorderTL * invCanvasSize2x;

		WidgetVec::const_iterator itChild = m_children.begin();
		WidgetVec::const_iterator enChild = m_children.end();

		while( itChild != enChild )
		{
			( *itChild )
				->_fillBuffersAndCommands( _vertexBuffer, _textVertBuffer, outerTopLeftWithClipping,
										   Ogre::Vector2::ZERO, finalRot );
			++itChild;
		}
	}
	//-------------------------------------------------------------------------
	void LabelBmp::_updateDirtyGlyphs() { updateGlyphs(); }
	//-------------------------------------------------------------------------
	bool LabelBmp::isLabelBmpDirty() const { return m_glyphsDirty; }
	//-------------------------------------------------------------------------
	void LabelBmp::flagDirty()
	{
		if( !isLabelBmpDirty() )
			m_manager->_addDirtyLabelBmp( this );
		m_glyphsDirty = true;
	}
	//-------------------------------------------------------------------------
	size_t LabelBmp::getMaxNumGlyphs() const
	{
		size_t retVal = m_shapes.size();

		const size_t maxGlyphs = retVal;

		if( m_shadowOutline )
			retVal += maxGlyphs;

		return retVal;
	}
	//-------------------------------------------------------------------------
	void LabelBmp::setText( const std::string &text, States::States forState )
	{
		if( forState == States::NumStates )
		{
			for( size_t i = 0; i < States::NumStates; ++i )
			{
				if( m_text[i] != text )
				{
					m_text[i] = text;
					if( i == m_currentState )
						flagDirty();
				}
			}
		}
		else
		{
			if( m_text[forState] != text )
			{
				m_text[forState] = text;
				if( forState == m_currentState )
					flagDirty();
			}
		}
	}
	//-------------------------------------------------------------------------
	const std::string &LabelBmp::getText( States::States state )
	{
		if( state == States::NumStates )
			state = m_currentState;
		return m_text[state];
	}
	//-------------------------------------------------------------------------
	void LabelBmp::sizeToFit()
	{
		if( m_glyphsDirty )
			updateGlyphs();

		if( m_shapes.empty() )
			return;

		Ogre::Vector2 maxSize( Ogre::Vector2::ZERO );

		float currentWidth = 0.0f;
		uint32_t numLines = 1u;
		uint32_t numColumns = 0u;
		BmpGlyphVec::const_iterator itor = m_shapes.begin();
		BmpGlyphVec::const_iterator endt = m_shapes.end();

		while( itor != endt )
		{
			if( itor->isNewline )
			{
				maxSize.x = std::max( maxSize.x, currentWidth );
				currentWidth = 0.0f;
				numColumns = 0u;
				++numLines;
			}
			else if( itor->isTab )
			{
				currentWidth = ceilf( ( currentWidth + itor->bmpChar->xadvance * 0.25f ) /
									  ( itor->bmpChar->xadvance * 2.0f ) ) *
							   ( itor->bmpChar->xadvance * 2.0f );
			}
			else
			{
				currentWidth += itor->bmpChar->xadvance;
			}

			++itor;
		}

		maxSize.x = std::max( maxSize.x, currentWidth );
		maxSize.y = ( m_shapes.back().bmpChar->yoffset + m_shapes.back().bmpChar->height ) *
					static_cast<Ogre::Real>( numLines );

		ShaperManager *shaperManager = m_manager->getShaperManager();
		const BmpFont *bmpFont = shaperManager->getBmpFont( m_font );
		const float fontScale = m_fontSize.asFloat() / bmpFont->getBakedFontSize().asFloat();

		maxSize *= fontScale;

		// Set new dimensions
		const Ogre::Vector2 canvasSize = m_manager->getCanvasSize();
		const Ogre::Vector2 invWindowRes = 0.5f * m_manager->getInvWindowResolution2x();

		m_size = maxSize * invWindowRes * canvasSize;
		m_size.x = std::ceil( m_size.x );
		m_size.y = std::ceil( m_size.y );
		m_minSize = m_size;
	}
	//-------------------------------------------------------------------------
	void LabelBmp::setState( States::States state, bool smartHighlight )
	{
		const States::States oldState = m_currentState;
		Renderable::setState( state, smartHighlight );

		if( oldState != state )
		{
			if( m_text[oldState] != m_text[state] )
				flagDirty();
		}
	}
}  // namespace Colibri
