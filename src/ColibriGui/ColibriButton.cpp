
#include "ColibriGui/ColibriButton.h"

#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"

namespace Colibri
{
	Button::Button( ColibriManager *manager ) :
		Renderable( manager ),
		m_label( 0 ),
		m_labelTopLeftMargin( Ogre::Vector2::ZERO ),
		m_labelBottomRightMargin( Ogre::Vector2::ZERO )
	{
		m_clickable = true;
		m_keyboardNavigable = true;
	}
	//-------------------------------------------------------------------------
	void Button::_initialize()
	{
		_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::Button ) );
		Renderable::_initialize();
	}
	//-------------------------------------------------------------------------
	void Button::_destroy()
	{
		Renderable::_destroy();

		// m_label is a child of us, so it will be destroyed by our super class
		m_label = 0;
	}
	//-------------------------------------------------------------------------
	void Button::setLabelMargins( const Ogre::Vector2 &labelTopLeftMargin,
								  const Ogre::Vector2 &labelBottomRightMargin )
	{
		m_labelTopLeftMargin = labelTopLeftMargin;
		m_labelBottomRightMargin = labelBottomRightMargin;
	}
	//-------------------------------------------------------------------------
	bool Button::hasLabel() const { return m_label != 0; }
	//-------------------------------------------------------------------------
	Label *Button::getLabel()
	{
		if( !m_label )
		{
			m_label = m_manager->createWidget<Label>( this );
			m_label->setSize( getSizeAfterClipping() );
			m_label->setTextHorizAlignment( TextHorizAlignment::Center );
			m_label->setTextVertAlignment( TextVertAlignment::Center );
		}

		return m_label;
	}
	//-------------------------------------------------------------------------
	void Button::sizeToFit( float maxAllowedWidth, TextHorizAlignment::TextHorizAlignment newHorizPos,
							TextVertAlignment::TextVertAlignment newVertPos, States::States baseState )
	{
		if( m_label )
			m_label->sizeToFit( maxAllowedWidth, newHorizPos, newVertPos, baseState );
		Ogre::Vector2 maxSize( calculateChildrenSize() + getBorderCombined() );
		if( m_label )
		{
			// If m_label has already been positioned, then calculateChildrenSize
			// already accounted for one m_label->m_margin + m_labelTopLeftMargin,
			// thus we must subtract it.
			// If m_label hasn't been positioned yet, then getLocalTopLeft will be
			// 0 and totalMargin will be correct.
			const Ogre::Vector2 totalMargin =
				m_label->m_margin * 2.0f + m_labelTopLeftMargin + m_labelBottomRightMargin;
			maxSize += totalMargin - m_label->getLocalTopLeft();
		}
		setSize( maxSize );
	}
	//-------------------------------------------------------------------------
	void Button::setTransformDirty( uint32_t dirtyReason )
	{
		if( m_label )
		{
			const Ogre::Vector2 totalMargin =
				m_label->m_margin * 2.0f + m_labelTopLeftMargin + m_labelBottomRightMargin;
			const Ogre::Vector2 sizeAfterClipping = getSizeAfterClipping() - totalMargin;
			if( m_label->getSize() != sizeAfterClipping )
			{
				m_label->setTopLeft( m_label->m_margin + m_labelTopLeftMargin );
				m_label->setSize( sizeAfterClipping );
			}
		}
		Renderable::setTransformDirty( dirtyReason );
	}
}  // namespace Colibri
