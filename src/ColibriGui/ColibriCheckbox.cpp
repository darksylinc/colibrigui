
#include "ColibriGui/ColibriCheckbox.h"

#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"

#include "OgreLwString.h"

namespace Colibri
{
	Checkbox::Checkbox( ColibriManager *manager ) :
		Widget( manager ),
		m_button( 0 ),
		m_tickmark( 0 ),
		m_currentValue( 0 ),
		m_stateMode( TwoState ),
		m_horizDir( HorizWidgetDir::AutoLTR ),
		m_mode( BigButton ),
		m_tickmarkMargin( manager->m_defaultTickmarkMargin ),
		m_tickmarkSize( manager->m_defaultTickmarkSize )
	{
		m_clickable = true;
		m_keyboardNavigable = true;
		m_pressable = true;
		m_childrenClickable = false;
	}
	//-------------------------------------------------------------------------
	void Checkbox::_initialize()
	{
		m_button = m_manager->createWidget<Button>( this );
		m_button->_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::Checkbox ) );

		for( size_t i = 0; i < 3u; ++i )
		{
			const SkinWidgetTypes::SkinWidgetTypes widgetType =
				static_cast<SkinWidgetTypes::SkinWidgetTypes>(
					SkinWidgetTypes::CheckboxTickmarkUnchecked + i );
			memcpy( m_skinPacks[i], m_manager->getDefaultSkin( widgetType ), sizeof( m_skinPacks[i] ) );
		}

		m_tickmark = m_manager->createWidget<Button>( this );
		m_tickmark->_setSkinPack( m_skinPacks[m_currentValue] );

		updateTickmark();

		Widget::_initialize();

		m_tickmark->addActionListener( this, ActionMask::PrimaryActionPerform );
		// Add ourselves as well
		this->addActionListener( this, ActionMask::PrimaryActionPerform );
	}
	//-------------------------------------------------------------------------
	void Checkbox::_destroy()
	{
		Widget::_destroy();

		// m_tickmark is a child of us, so it will be destroyed by our super class
		m_tickmark = 0;
		// m_button is a child of us, so it will be destroyed by our super class
		m_button = 0;
	}
	//-------------------------------------------------------------------------
	void Checkbox::setSkinPack( Ogre::IdString skinPackName ) { m_button->setSkinPack( skinPackName ); }
	//-------------------------------------------------------------------------
	void Checkbox::setTickmarkSkinPack( uint8_t stateValue, Ogre::IdString skinPackName )
	{
		COLIBRI_ASSERT_LOW( stateValue < 3u );
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinPack *pack = skinManager->findSkinPack( skinPackName );
		if( pack )
		{
			for( size_t i = 0; i < States::NumStates; ++i )
			{
				const SkinInfo *skin = skinManager->findSkin( *pack, static_cast<States::States>( i ) );
				if( skin )
					m_skinPacks[stateValue][i] = skin;
			}

			if( stateValue == m_currentValue )
				m_tickmark->_setSkinPack( m_skinPacks[m_currentValue] );
		}
	}
	//-------------------------------------------------------------------------
	void Checkbox::setTickmarkSkinPack(
		uint8_t stateValue, SkinInfo const *colibri_nonnull const *colibri_nullable skinInfos )
	{
		COLIBRI_ASSERT_LOW( stateValue < 3u );
		for( size_t i = 0; i < States::NumStates; ++i )
			m_skinPacks[stateValue][i] = skinInfos[i];

		if( stateValue == m_currentValue )
			m_tickmark->_setSkinPack( m_skinPacks[m_currentValue] );
	}
	//-------------------------------------------------------------------------
	void Checkbox::setTickmarkSkin( uint8_t stateValue, Ogre::IdString skinName,
									States::States forState )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinInfoMap &skins = skinManager->getSkins();

		SkinInfoMap::const_iterator itor = skins.find( skinName );
		if( itor != skins.end() )
		{
			if( forState == States::NumStates )
			{
				for( size_t i = 0; i < States::NumStates; ++i )
					m_skinPacks[stateValue][i] = &itor->second;
			}
			else
			{
				m_skinPacks[stateValue][forState] = &itor->second;
			}
		}

		if( stateValue == m_currentValue )
			m_tickmark->_setSkinPack( m_skinPacks[m_currentValue] );
	}
	//-------------------------------------------------------------------------
	void Checkbox::setCheckboxMode( Checkbox::Mode mode )
	{
		m_mode = mode;
		updateTickmark();
	}
	//-------------------------------------------------------------------------
	void Checkbox::setTickmarkMarginAndSize( float margin, const Ogre::Vector2 &size )
	{
		m_tickmarkMargin = margin;
		m_tickmarkSize = size;
		updateTickmark();
	}
	//-------------------------------------------------------------------------
	void Checkbox::setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir )
	{
		m_horizDir = horizWidgetDir;
		updateTickmark();
	}
	//-------------------------------------------------------------------------
	void Checkbox::updateTickmark()
	{
		if( !m_tickmark )
			return;  //_initialize hasn't been called yet

		const Ogre::Vector2 checkboxSize( getSizeAfterClipping() );
		const float margin = m_tickmarkMargin;
		const Ogre::Vector2 tickmarkSize( m_tickmarkSize );

		m_tickmark->setSize( tickmarkSize );

		const bool rightToLeft = m_manager->shouldSwapRTL( m_horizDir );

		m_button->setLabelMargins( Ogre::Vector2::ZERO, Ogre::Vector2::ZERO );
		if( m_mode == TickButton )
		{
			const Ogre::Vector2 buttonSize( checkboxSize.x - tickmarkSize.x - margin * 2.0f,
											checkboxSize.y );
			m_button->setSize( buttonSize );

			if( rightToLeft )
			{
				m_tickmark->setTopLeft( Ogre::Vector2( checkboxSize.x - tickmarkSize.x - margin,
													   buttonSize.y * 0.5f - tickmarkSize.y * 0.5f ) );
				m_button->setTopLeft(
					Ogre::Vector2( m_tickmark->getLocalTopLeft().x - margin - buttonSize.x, 0.0f ) );
			}
			else
			{
				m_tickmark->setTopLeft(
					Ogre::Vector2( margin, buttonSize.y * 0.5f - tickmarkSize.y * 0.5f ) );
				m_button->setTopLeft( Ogre::Vector2( tickmarkSize.x + margin * 2.0f, 0.0f ) );
			}
		}
		else
		{
			const Ogre::Vector2 buttonSize( checkboxSize );
			m_button->setSize( buttonSize );

			if( rightToLeft )
			{
				// Text inside the button should NOT overlap with the
				// tickmark when the text isn't centered
				if( m_button->hasLabel() &&
					m_button->getLabel()->getTextHorizAlignment() != TextHorizAlignment::Center )
				{
					m_button->setLabelMargins(
						Ogre::Vector2::ZERO,
						Ogre::Vector2( m_tickmarkMargin * 2.0f + tickmarkSize.x, 0.0f ) );
				}

				m_tickmark->setTopLeft( Ogre::Vector2( checkboxSize.x - tickmarkSize.x - margin,
													   buttonSize.y * 0.5f - tickmarkSize.y * 0.5f ) );
				m_button->setTopLeft( Ogre::Vector2::ZERO );
			}
			else
			{
				// Text inside the button should NOT overlap with the
				// tickmark when the text isn't centered
				if( m_button->hasLabel() &&
					m_button->getLabel()->getTextHorizAlignment() != TextHorizAlignment::Center )
				{
					m_button->setLabelMargins(
						Ogre::Vector2( m_tickmarkMargin * 2.0f + tickmarkSize.x, 0.0f ),
						Ogre::Vector2::ZERO );
				}

				m_tickmark->setTopLeft(
					Ogre::Vector2( margin, buttonSize.y * 0.5f - tickmarkSize.y * 0.5f ) );
				m_button->setTopLeft( Ogre::Vector2::ZERO );
			}
		}

		m_button->updateDerivedTransformFromParent( false );
		m_tickmark->updateDerivedTransformFromParent( false );

		m_tickmark->_setSkinPack( m_skinPacks[m_currentValue] );
	}
	//-------------------------------------------------------------------------
	void Checkbox::setCurrentValue( uint8_t currentValue )
	{
		currentValue = Ogre::Math::Clamp<uint8_t>( currentValue, 0u, getMaxValue() );

		if( m_currentValue != currentValue )
		{
			m_currentValue = currentValue;
			updateTickmark();
		}
	}
	//-------------------------------------------------------------------------
	void Checkbox::setTriState( bool triState ) { setStateMode( triState ? TriState : TwoState ); }
	//-------------------------------------------------------------------------
	void Checkbox::setStateMode( StateMode stateMode )
	{
		m_stateMode = stateMode;
		m_currentValue = Ogre::Math::Clamp<uint8_t>( m_currentValue, 0u, getMaxValue() );
		updateTickmark();
	}
	//-------------------------------------------------------------------------
	void Checkbox::sizeToFit( float maxAllowedWidth, TextHorizAlignment::TextHorizAlignment newHorizPos,
							  TextVertAlignment::TextVertAlignment newVertPos, States::States baseState )
	{
		m_button->sizeToFit( maxAllowedWidth, newHorizPos, newVertPos, baseState );
		const Ogre::Vector2 buttonSize = m_button->getSize();

		Ogre::Vector2 newSize = buttonSize;

		if( m_mode == TickButton )
			newSize.x = m_tickmarkSize.x + m_tickmarkMargin * 2.0f + buttonSize.x;
		else
			newSize.x = ( m_tickmarkSize.x + m_tickmarkMargin * 2.0f ) * 2.0f + buttonSize.x;
		newSize.y = std::max( m_tickmarkSize.y, newSize.y );
		setSize( newSize );
	}
	//-------------------------------------------------------------------------
	void Checkbox::setTransformDirty( uint32_t dirtyReason )
	{
		// Only update the tickmark if our size is directly being changed, not our parent's
		if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) ==
			TransformDirtyScale )
		{
			updateTickmark();
		}

		Widget::setTransformDirty( dirtyReason );
	}
	//-------------------------------------------------------------------------
	void Checkbox::notifyWidgetAction( Widget *widget, Action::Action action )
	{
		if( action == Action::PrimaryActionPerform )
		{
			if( widget == this || widget == m_tickmark ||
				( widget == m_button && m_currentValue == NoState ) )
			{
				m_currentValue = ( m_currentValue + 1u ) % ( getMaxValue() + 1u );
				if( m_currentValue != NoState )
					updateTickmark();
				if( widget != this )
					_callActionListeners( Action::PrimaryActionPerform );
			}
		}
	}
}  // namespace Colibri
