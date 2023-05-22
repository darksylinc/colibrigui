
#include "ColibriGui/ColibriToggleButton.h"

#include "ColibriGui/ColibriButton.h"
#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/ColibriSkinManager.h"

#include "OgreLwString.h"

namespace Colibri
{
	ToggleButton::ToggleButton( ColibriManager *manager ) : Button( manager ), m_currentValue( 0u ) {}
	//-------------------------------------------------------------------------
	void ToggleButton::_initialize()
	{
		for( size_t i = 0; i < 2u; ++i )
		{
			const SkinWidgetTypes::SkinWidgetTypes widgetType =
				static_cast<SkinWidgetTypes::SkinWidgetTypes>( SkinWidgetTypes::ToggleButtonUnchecked +
															   i );
			memcpy( m_skinPacks[i], m_manager->getDefaultSkin( widgetType ), sizeof( m_skinPacks[i] ) );
		}

		this->_setSkinPack( m_skinPacks[m_currentValue] );

		Widget::_initialize();

		this->addActionListener( this, ActionMask::PrimaryActionPerform );
	}
	//-------------------------------------------------------------------------
	void ToggleButton::setSkinPack( uint8_t stateValue, Ogre::IdString skinPackName )
	{
		SkinManager *skinManager = m_manager->getSkinManager();
		const SkinPack *pack = skinManager->findSkinPack( skinPackName );
		if( pack )
		{
			for( size_t i = 0; i < States::NumStates; ++i )
			{
				const SkinInfo *skin = skinManager->findSkin( *pack, static_cast<States::States>( i ) );
				m_skinPacks[stateValue][i] = skin;
			}

			if( m_currentValue == stateValue )
				this->_setSkinPack( m_skinPacks[stateValue] );
		}
	}
	//-------------------------------------------------------------------------
	void ToggleButton::setSkin( uint8_t stateValue, Ogre::IdString skinName, States::States forState )
	{
		COLIBRI_ASSERT_LOW( stateValue < 3u );

		if( skinName == Ogre::IdString() )
		{
			if( forState == States::NumStates )
			{
				for( size_t i = 0; i < States::NumStates; ++i )
					m_skinPacks[stateValue][i] = nullptr;
			}
			else
			{
				m_skinPacks[stateValue][forState] = nullptr;
			}
		}
		else
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
				_setSkinPack( m_skinPacks[m_currentValue] );
		}
	}
	//-------------------------------------------------------------------------
	void ToggleButton::setCurrentValue( uint8_t currentValue )
	{
		currentValue = Ogre::Math::Clamp<uint8_t>( currentValue, 0u, getMaxValue() );

		if( m_currentValue != currentValue )
		{
			m_currentValue = currentValue;
			_setSkinPack( m_skinPacks[m_currentValue] );
		}
	}
	//-------------------------------------------------------------------------
	void ToggleButton::notifyWidgetAction( Widget *widget, Action::Action action )
	{
		if( action == Action::PrimaryActionPerform )
		{
			m_currentValue = ( m_currentValue + 1u ) % ( getMaxValue() + 1u );
			_setSkinPack( m_skinPacks[m_currentValue] );
			if( widget != this )
				_callActionListeners( Action::PrimaryActionPerform );
		}
	}
}  // namespace Colibri
