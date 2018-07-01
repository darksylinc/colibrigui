
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
		m_multiline( false )
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
				m_manager->_addUpdateWidget( this );
			else
				m_manager->_removeUpdateWidget( this );
		}
	}
	//-------------------------------------------------------------------------
	void Editbox::_update( float timeSinceLast )
	{
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
			const std::string &oldText = m_label->getText();
			UnicodeString uStr( UnicodeString::fromUTF8( oldText ) );

			//Convert m_cursorPos from code points to code units
			int32_t offsetStart, offsetEnd;

			if( keyCode == KeyCode::Backspace )
			{
				offsetEnd	= uStr.moveIndex32( 0, static_cast<int32_t>( m_cursorPos ) );
				offsetStart	= uStr.moveIndex32( offsetEnd, -1 );

				//Set the new cursor position
				if( m_cursorPos )
					--m_cursorPos;
			}
			else
			{
				offsetStart	= uStr.moveIndex32( 0, static_cast<int32_t>( m_cursorPos ) );
				offsetEnd	= uStr.moveIndex32( offsetStart, 1 );
			}

			uStr.remove( offsetStart, offsetEnd - offsetStart );

			//Convert back to UTF8
			std::string result;
			uStr.toUTF8String( result );
			m_label->setText( result );
		}
		else if( keyCode == KeyCode::Home )
		{
			m_cursorPos = 0;
		}
		else if( keyCode == KeyCode::End )
		{
			m_cursorPos = static_cast<uint32_t>( m_label->getGlyphCount() );
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

		//Convert m_cursorPos from code points to code units
		const int32_t offset = uStr.moveIndex32( 0, static_cast<int32_t>( m_cursorPos ) );
		//Append the text
		uStr.insert( offset, appendText );

		//Convert back to UTF8
		std::string result;
		uStr.toUTF8String( result );
		m_label->setText( result );

		//Advance the cursor
		m_cursorPos += appendText.countChar32();
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
				++m_cursorPos;
			else
			{
				if( m_cursorPos )
					--m_cursorPos;
			}
			callActionListeners( Action::ValueChanged );
		}
		else if( direction == Borders::Right )
		{
			if( m_manager->swapRTLControls() )
			{
				if( m_cursorPos )
					--m_cursorPos;
			}
			else
				++m_cursorPos;
			callActionListeners( Action::ValueChanged );
		}
	}
}
