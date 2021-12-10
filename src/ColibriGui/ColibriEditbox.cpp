
#include "ColibriGui/ColibriEditbox.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"

#include "unicode/unistr.h"

#define TODO_text_edit

namespace Colibri
{
	Editbox::Editbox( ColibriManager *manager ) :
		Renderable( manager ),
		m_label( 0 ),
		m_caret( 0 ),
		m_secureLabel( 0 ),
		m_cursorPos( 0 ),
#if defined( __ANDROID__ ) || ( defined( __APPLE__ ) && defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE )
		m_inputType( InputType::Text ),
#endif
		m_multiline( false ),
		m_blinkTimer( 0 )
	{
		m_clickable = true;
		m_keyboardNavigable = true;
		m_mouseReleaseTriggersPrimaryAction = false;

		m_autoSetNextWidget[Borders::Left] = false;
		m_autoSetNextWidget[Borders::Right] = false;
	}
	//-------------------------------------------------------------------------
	void Editbox::_initialize()
	{
		_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::Editbox ) );

		m_label = m_manager->createWidget<Label>( this );
		m_label->setSize( getSizeAfterClipping() );
		m_label->setTextHorizAlignment( TextHorizAlignment::Natural );
		m_label->setTextVertAlignment( TextVertAlignment::Top );

		m_caret = m_manager->createWidget<Label>( this );
		m_caret->setTextHorizAlignment( TextHorizAlignment::Left );
		m_caret->setTextVertAlignment( TextVertAlignment::Top );
		m_caret->setText( "|" );
		m_caret->sizeToFit( States::Idle );
		m_caret->setHidden( true );

		Renderable::_initialize();
	}
	//-------------------------------------------------------------------------
	void Editbox::_destroy()
	{
		if( requiresActiveUpdate() )
			m_manager->_removeUpdateWidget( this );

		Renderable::_destroy();

		// m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	void Editbox::showCaret()
	{
		m_caret->setHidden( false );
		m_blinkTimer = 0;
	}
	//-------------------------------------------------------------------------
	bool Editbox::requiresActiveUpdate() const
	{
		return m_currentState == States::HighlightedButton ||
			   m_currentState == States::HighlightedButtonAndCursor || m_currentState == States::Pressed;
	}
	//-------------------------------------------------------------------------
	void Editbox::syncSecureLabel()
	{
		if( !m_secureLabel )
			return;

		const size_t numGlyphs = m_label->getGlyphCount();
		std::string secureText;
		secureText.resize( numGlyphs, '*' );
		m_secureLabel->setText( secureText );
		m_manager->_updateDirtyLabels();
	}
	//-------------------------------------------------------------------------
	void Editbox::setText( const char *text )
	{
		m_label->setText( text );
		// Set the cursor at the end (will later be clamped correctly)
		m_cursorPos = std::numeric_limits<uint32_t>::max();

		if( m_secureLabel )
		{
			m_manager->_updateDirtyLabels();
			syncSecureLabel();
		}
	}
	//-------------------------------------------------------------------------
	const std::string &Editbox::getText() const { return m_label->getText(); }
	//-------------------------------------------------------------------------
	void Editbox::setState( States::States state, bool smartHighlight )
	{
		const bool wasActive = requiresActiveUpdate();

		if( m_currentState != state && state == States::Pressed )
		{
			ColibriListener *colibriListener = m_manager->getColibriListener();
			colibriListener->showTextInput( this );
		}

		Renderable::setState( state, smartHighlight );

		const bool isActive = requiresActiveUpdate();

		if( wasActive != isActive )
		{
			if( isActive )
			{
				m_manager->_addUpdateWidget( this );
				showCaret();
			}
			else
			{
				m_manager->_removeUpdateWidget( this );
				m_caret->setHidden( true );
				m_blinkTimer = 0;
			}
		}
	}
	//-------------------------------------------------------------------------
	void Editbox::setSecureEntry( const bool bSecureEntry )
	{
		if( bSecureEntry )
		{
			if( !m_secureLabel )
			{
				m_secureLabel = m_manager->createWidget<Label>( this );
				m_secureLabel->setSize( getSizeAfterClipping() );
				m_secureLabel->setTextHorizAlignment( TextHorizAlignment::Natural );
				m_secureLabel->setTextVertAlignment( TextVertAlignment::Top );

				m_label->setHidden( true );

				syncSecureLabel();
			}
		}
		else
		{
			if( m_secureLabel )
			{
				m_manager->destroyWidget( m_secureLabel );
				m_secureLabel = 0;
				m_label->setHidden( false );
			}
		}
	}
	//-------------------------------------------------------------------------
	bool Editbox::isSecureEntry() const { return m_secureLabel != 0; }
	//-------------------------------------------------------------------------
	void Editbox::setInputType( InputType::InputType inputType, const std::string &textHint )
	{
#if defined( __ANDROID__ ) || ( defined( __APPLE__ ) && defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE )
		m_inputType = inputType;
		m_textHint = textHint;
#endif
	}
	//-------------------------------------------------------------------------
	InputType::InputType Editbox::getInputType() const
	{
		if( isSecureEntry() )
			return InputType::Password;
		else if( isTextMultiline() )
			return InputType::Multiline;
#if defined( __ANDROID__ ) || ( defined( __APPLE__ ) && defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE )
		return m_inputType;
#else
		return InputType::Text;
#endif
	}
	//-------------------------------------------------------------------------
	std::string Editbox::getTextHint() const
	{
#if defined( __ANDROID__ ) || ( defined( __APPLE__ ) && defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE )
		return m_textHint;
#else
		return "";
#endif
	}
	//-------------------------------------------------------------------------
	void Editbox::_update( float timeSinceLast )
	{
		m_blinkTimer += timeSinceLast;

		if( m_blinkTimer >= 0.5f )
		{
			m_caret->setHidden( !m_caret->isHidden() );
			m_blinkTimer = 0.0f;
		}

		m_cursorPos = std::min<uint32_t>( m_cursorPos, (uint32_t)m_label->getGlyphCount() );

		syncSecureLabel();

		const Label *labelForCaret = m_secureLabel ? m_secureLabel : m_label;

		FontSize ptSize;
		uint16_t font;
		Ogre::Vector2 pos = labelForCaret->getCaretTopLeft( m_cursorPos, ptSize, font );

		// Subtract the caret's bearing so it appears at the beginning
		m_caret->setDefaultFontSize( ptSize );
		m_caret->setDefaultFont( font );
		if( m_caret->isAnyStateDirty() )
			m_manager->_updateDirtyLabels();
		m_caret->setTopLeft( Ogre::Vector2::ZERO );
		const Ogre::Vector2 caretBearing = m_caret->getCaretTopLeft( 0u, ptSize, font );

		m_caret->setTopLeft( pos - caretBearing * 2.0f );

		if( m_caret->getDefaultFontSize() != ptSize )
		{
			m_caret->setDefaultFontSize( ptSize );
			m_caret->sizeToFit( States::Idle );
		}
	}
	//-------------------------------------------------------------------------
	void Editbox::_setTextEdit( const char *text, int32_t selectStart, int32_t selectLength )
	{
		TODO_text_edit;
	}
	//------------------------------------------------------------------------
	void Editbox::_setTextSpecialKey( uint32_t keyCode, uint16_t keyMod, size_t repetition )
	{
		COLIBRI_ASSERT_LOW( repetition > 0u );

		if( keyCode == KeyCode::Backspace || keyCode == KeyCode::Delete )
		{
			bool isAtLimit = ( keyCode == KeyCode::Backspace && m_cursorPos == 0 ) ||
							 ( keyCode == KeyCode::Delete && m_cursorPos >= m_label->getGlyphCount() );

			if( !isAtLimit )
			{
				const std::string &oldText = m_label->getText();
				UnicodeString uStr( UnicodeString::fromUTF8( oldText ) );

				uint32_t lastCursorPosToDelete = m_cursorPos;
				if( keyCode == KeyCode::Backspace )
				{
					for( size_t i = 0u; i < repetition; ++i )
					{
						// Set the new cursor position
						m_cursorPos = m_label->regressGlyphToPreviousCluster( m_cursorPos );
						if( i == 0u )
							lastCursorPosToDelete = m_cursorPos;
					}
				}
				else
				{
					for( size_t i = 0u; i < repetition - 1u; ++i )
					{
						lastCursorPosToDelete =
							m_label->advanceGlyphToNextCluster( lastCursorPosToDelete );
					}
				}

				size_t firstGlyphStart, firstGlyphLength;
				size_t lastGlyphStart, lastGlyphLength;
				m_label->getGlyphStartUtf16( m_cursorPos, firstGlyphStart, firstGlyphLength );
				m_label->getGlyphStartUtf16( lastCursorPosToDelete, lastGlyphStart, lastGlyphLength );

				size_t glyphLength = lastGlyphStart + lastGlyphLength - firstGlyphStart;
				if( glyphLength > 0 )
				{
					uStr.remove( static_cast<int32_t>( firstGlyphStart ),
								 static_cast<int32_t>( glyphLength ) );
				}

				// Convert back to UTF8
				std::string result;
				uStr.toUTF8String( result );
				m_label->setText( result );
				m_manager->callActionListeners( this, Action::ValueChanged );
			}
			else if( m_label->getGlyphCount() == 0u && !m_label->getText().empty() )
			{
				// TODO: Some non-renderable UTF character got stuck in the text
				// We can't navigate it because it's not a glyph. Just clear the string
				// to restore the widget to a working state
				// It appears 'RTL start' code points can trigger this.
				m_label->setText( "" );
				m_manager->callActionListeners( this, Action::ValueChanged );
			}

			showCaret();
		}
		else if( keyCode == KeyCode::Home )
		{
			m_cursorPos = 0;
			showCaret();
		}
		else if( keyCode == KeyCode::End )
		{
			m_cursorPos = static_cast<uint32_t>( m_label->getGlyphCount() );
			showCaret();
		}
		else if( keyCode == KeyCode::Tab || keyCode == KeyCode::Enter )
		{
			const size_t c_maxRepetition = 512u;
			char inBuffer[c_maxRepetition + 1u];
			size_t maxRepetition = std::min( c_maxRepetition, repetition );
			if( keyCode == KeyCode::Tab )
			{
				for( size_t i = 0u; i < maxRepetition; ++i )
					inBuffer[i] = '\t';
			}
			else if( keyCode == KeyCode::Enter )
			{
				for( size_t i = 0u; i < maxRepetition; ++i )
					inBuffer[i] = '\n';
			}
			inBuffer[maxRepetition] = '\0';
			_setTextInput( inBuffer, false );
		}
		else if( keyCode == 'c' && ( keyMod & ( KeyMod::LCtrl | KeyMod::RCtrl ) ) )
		{
			if( !isSecureEntry() )
			{
				ColibriListener *colibriListener = m_manager->getColibriListener();
				colibriListener->setClipboardText( m_label->getText().c_str() );
			}
		}
		else if( keyCode == 'v' && ( keyMod & ( KeyMod::LCtrl | KeyMod::RCtrl ) ) )
		{
			ColibriListener *colibriListener = m_manager->getColibriListener();
			char *clipboardText = 0;
			if( colibriListener->getClipboardText( &clipboardText ) )
			{
				for( size_t i = 0u; i < repetition; ++i )
					_setTextInput( clipboardText, false );
				colibriListener->freeClipboardText( clipboardText );
				m_manager->callActionListeners( this, Action::ValueChanged );
			}
		}
	}
	//-------------------------------------------------------------------------
	void Editbox::_setTextInput( const char *text, const bool bReplaceContents,
								 const bool bCallActionListener )
	{
		size_t oldGlyphCount = 0u;

		if( !bReplaceContents )
		{
			const std::string &oldText = m_label->getText();
			UnicodeString uStr( UnicodeString::fromUTF8( oldText ) );
			UnicodeString appendText( UnicodeString::fromUTF8( text ) );

			oldGlyphCount = m_label->getGlyphCount();

			// Convert m_cursorPos from glyph to code units
			size_t glyphStart;
			size_t glyphLength;
			m_label->getGlyphStartUtf16( m_cursorPos, glyphStart, glyphLength );

			// Append the text
			uStr.insert( glyphStart, appendText );

			// Convert back to UTF8
			std::string result;
			uStr.toUTF8String( result );
			m_label->setText( result );
		}
		else
		{
			m_label->setText( text );
		}

		// We must update now, otherwise if _setTextInput gets called, getGlyphStartUtf16 will be wrong
		m_manager->_updateDirtyLabels();

		const size_t newGlyphCount = m_label->getGlyphCount();

		// Advance the cursor
		m_cursorPos += newGlyphCount - oldGlyphCount;

		showCaret();

		if( bCallActionListener )
			m_manager->callActionListeners( this, Action::ValueChanged );
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Editbox::_getImeLocation()
	{
		m_caret->updateDerivedTransformFromParent();
		return m_caret->getDerivedBottomRight();
	}
	//-------------------------------------------------------------------------
	bool Editbox::isTextMultiline() const { return m_multiline; }
	//-------------------------------------------------------------------------
	bool Editbox::wantsTextInput() const { return true; }
	//-------------------------------------------------------------------------
	Label *Editbox::getLabel() { return m_label; }
	//-------------------------------------------------------------------------
	void Editbox::setTransformDirty( uint32_t dirtyReason )
	{
		const Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping();
		if( m_label && m_label->getSize() != sizeAfterClipping )
			m_label->setSize( sizeAfterClipping );
		if( m_secureLabel && m_secureLabel->getSize() != sizeAfterClipping )
			m_secureLabel->setSize( sizeAfterClipping );
		Renderable::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Editbox::_notifyActionKeyMovement( Borders::Borders direction )
	{
		if( direction == Borders::Left )
		{
			if( m_manager->swapRTLControls() )
				m_cursorPos = m_label->advanceGlyphToNextCluster( m_cursorPos );
			else
				m_cursorPos = m_label->regressGlyphToPreviousCluster( m_cursorPos );
			showCaret();
			_callActionListeners( Action::ValueChanged );
		}
		else if( direction == Borders::Right )
		{
			if( m_manager->swapRTLControls() )
				m_cursorPos = m_label->regressGlyphToPreviousCluster( m_cursorPos );
			else
				m_cursorPos = m_label->advanceGlyphToNextCluster( m_cursorPos );

			showCaret();
			_callActionListeners( Action::ValueChanged );
		}
	}
}  // namespace Colibri
