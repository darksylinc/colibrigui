
#include "ColibriGui/ColibriEditbox.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriLabel.h"

namespace Colibri
{
	Editbox::Editbox( ColibriManager *manager ) :
		Renderable( manager ),
		m_label( 0 ),
		m_caret( 0 ),
		m_cursorPos( 0 )
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
		m_caret->setSize( m_manager->getCanvasSize() );
		m_caret->setTextHorizAlignment( TextHorizAlignment::Left );
		m_caret->setTextVertAlignment( TextVertAlignment::Top );
		m_caret->setText( "|" );

		m_label->setText( "Hel lo" );

		Renderable::_initialize();
	}
	//-------------------------------------------------------------------------
	void Editbox::_destroy()
	{
		Renderable::_destroy();

		//m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	void Editbox::update()
	{
		m_cursorPos = std::min<uint32_t>( m_cursorPos, (uint32_t)m_label->getGlyphCount() );

		FontSize ptSize;

		//Subtract the caret's bearing so it appears at the beginning
		m_caret->setTopLeft( Ogre::Vector2::ZERO );
		const Ogre::Vector2 caretBearing = m_caret->getCaretTopLeft( 0u, ptSize );

		Ogre::Vector2 pos = m_label->getCaretTopLeft( m_cursorPos, ptSize );
		m_caret->setTopLeft( pos - caretBearing * 2.0f );
		m_caret->setDefaultFontSize( ptSize );
	}
	//-------------------------------------------------------------------------
	void Editbox::_setTextEdit( const char *text, int32_t selectStart, int32_t selectLength )
	{
	}
	//-------------------------------------------------------------------------
	void Editbox::_setTextInput( const char *text )
	{
		std::string newText = m_label->getText();
		const size_t oldSize = newText.size();
		m_cursorPos = std::min<uint32_t>( m_cursorPos, (uint32_t)oldSize );
		newText.insert( m_cursorPos, text );
		m_label->setText( newText );
		m_cursorPos += newText.size() - oldSize;
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
