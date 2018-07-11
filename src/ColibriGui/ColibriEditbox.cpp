
#include "ColibriGui/ColibriEditbox.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriLabel.h"

#include "unicode/unistr.h"

#define TODO_text_edit

namespace Colibri
{
	Editbox::Editbox( ColibriManager *manager ) :
		Renderable( manager ),
		m_label( 0 ),
		m_caret( 0 ),
		m_cursorPos( 0 ),
		m_multiline( false ),
		m_blinkTimer( 0 )
	{
		m_clickable = true;
		m_keyboardNavigable = true;

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

		m_label->setText( "Hel lo" );

		Renderable::_initialize();
	}
	//-------------------------------------------------------------------------
	void Editbox::_destroy()
	{
		if( requiresActiveUpdate() )
			m_manager->_removeUpdateWidget( this );

		Renderable::_destroy();

		//m_label is a child of us, so it will be destroyed by our super class
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
		return	m_currentState == States::HighlightedButton ||
				m_currentState == States::HighlightedButtonAndCursor ||
				m_currentState == States::Pressed;
	}
	//-------------------------------------------------------------------------
	void Editbox::setState( States::States state, bool smartHighlight,
							bool broadcastEnable )
	{
		const bool wasActive = requiresActiveUpdate();

		Renderable::setState( state, smartHighlight, broadcastEnable );

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
	void Editbox::_update( float timeSinceLast )
	{
		m_blinkTimer += timeSinceLast;

		if( m_blinkTimer >= 0.5f )
		{
			m_caret->setHidden( !m_caret->isHidden() );
			m_blinkTimer = 0.0f;
		}

		m_cursorPos = std::min<uint32_t>( m_cursorPos, (uint32_t)m_label->getGlyphCount() );

		FontSize ptSize;

		//Subtract the caret's bearing so it appears at the beginning
		m_caret->setTopLeft( Ogre::Vector2::ZERO );
		const Ogre::Vector2 caretBearing = m_caret->getCaretTopLeft( 0u, ptSize );

		Ogre::Vector2 pos = m_label->getCaretTopLeft( m_cursorPos, ptSize );
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
	void Editbox::_setTextSpecialKey( uint32_t keyCode, uint16_t keyMod )
	{
		if( keyCode == KeyCode::Backspace || keyCode == KeyCode::Delete )
		{
			bool isAtLimit = (keyCode == KeyCode::Backspace && m_cursorPos == 0) ||
							 (keyCode == KeyCode::Delete && m_cursorPos >= m_label->getGlyphCount());

			if( !isAtLimit )
			{
				const std::string &oldText = m_label->getText();
				UnicodeString uStr( UnicodeString::fromUTF8( oldText ) );

				if( keyCode == KeyCode::Backspace )
				{
					//Set the new cursor position
					m_cursorPos = m_label->regressGlyphToPreviousCluster( m_cursorPos );
				}

				size_t glyphStart;
				size_t glyphLength;
				m_label->getGlyphStartUtf16( m_cursorPos, glyphStart, glyphLength );
				if( glyphLength > 0 )
				{
					uStr.remove( static_cast<int32_t>( glyphStart ),
								 static_cast<int32_t>( glyphLength ) );
				}

				//Convert back to UTF8
				std::string result;
				uStr.toUTF8String( result );
				m_label->setText( result );
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
		else if( keyCode == KeyCode::Tab )
		{
			_setTextInput( "\t" );
		}
		else if( keyCode == KeyCode::Enter )
		{
			_setTextInput( "\n" );
		}
		else if( keyCode == 'c' && (keyMod & (KeyMod::LCtrl|KeyMod::RCtrl)) )
		{
			ColibriListener *colibriListener = m_manager->getColibriListener();
			colibriListener->setClipboardText( m_label->getText().c_str() );
		}
		else if( keyCode == 'v' && (keyMod & (KeyMod::LCtrl|KeyMod::RCtrl)) )
		{
			ColibriListener *colibriListener = m_manager->getColibriListener();
			char *clipboardText = 0;
			if( colibriListener->getClipboardText( &clipboardText ) )
				_setTextInput( clipboardText );
		}
	}
	//-------------------------------------------------------------------------
	void Editbox::_setTextInput( const char *text )
	{
		const std::string &oldText = m_label->getText();
		UnicodeString uStr( UnicodeString::fromUTF8( oldText ) );
		UnicodeString appendText( UnicodeString::fromUTF8( text ) );

		const size_t oldGlyphCount = m_label->getGlyphCount();

		//Convert m_cursorPos from glyph to code units
		size_t glyphStart;
		size_t glyphLength;
		m_label->getGlyphStartUtf16( m_cursorPos, glyphStart, glyphLength );

		//Append the text
		uStr.insert( glyphStart, appendText );

		//Convert back to UTF8
		std::string result;
		uStr.toUTF8String( result );
		m_label->setText( result );

		//We must update now, otherwise if _setTextInput gets called, getGlyphStartUtf16 will be wrong
		m_manager->_updateDirtyLabels();

		const size_t newGlyphCount = m_label->getGlyphCount();

		//Advance the cursor
		m_cursorPos += newGlyphCount - oldGlyphCount;

		showCaret();
	}
	//-------------------------------------------------------------------------
	Ogre::Vector2 Editbox::_getImeLocation()
	{
		m_caret->updateDerivedTransformFromParent();
		return m_caret->getDerivedBottomRight();
	}
	//-------------------------------------------------------------------------
	bool Editbox::isTextMultiline() const
	{
		return m_multiline;
	}
	//-------------------------------------------------------------------------
	bool Editbox::wantsTextInput() const
	{
		return true;
	}
	//-------------------------------------------------------------------------
	Label* Editbox::getLabel()
	{
		return m_label;
	}
	//-------------------------------------------------------------------------
	void Editbox::setTransformDirty()
	{
		if( m_label && m_label->getSize() != m_size )
			m_label->setSize( getSizeAfterClipping() );
		Renderable::setTransformDirty();
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
			callActionListeners( Action::ValueChanged );
		}
		else if( direction == Borders::Right )
		{
			if( m_manager->swapRTLControls() )
				m_cursorPos = m_label->regressGlyphToPreviousCluster( m_cursorPos );
			else
				m_cursorPos = m_label->advanceGlyphToNextCluster( m_cursorPos );

			showCaret();
			callActionListeners( Action::ValueChanged );
		}
	}
}
