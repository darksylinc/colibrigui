
#pragma once

#include "ColibriGui/ColibriButton.h"

#include "OgreIdString.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class ToggleButton
		It's the same as a two-state Checkbox, but instead of having a tickmark,
		it's a regular Button that changes Skin Packs depending on whether it's checked or not.
	*/
	class ToggleButton : public Button, WidgetActionListener
	{
	protected:
		uint8_t m_currentValue;

		/// Each skin pack per m_skinPacks[m_currentValue]
		SkinInfo const *colibri_nullable m_skinPacks[2][States::NumStates];

	public:
		ToggleButton( ColibriManager *manager );

		void _initialize() override;

		/** Sets the skin packs for each state (unchecked, checked, tri-checked)
		@param stateValue
			Value in range [0; 2).
			0: Unchecked, 1: Checked
		@param skinPackName
			Skin pack to set.

			If an empty name is set IdString(), then when the ToggleButton goes into this state,
			the skin won't be altered, meaning it will continue to use the skin it was using.
		*/
		void setSkinPack( uint8_t stateValue, Ogre::IdString skinPackName );

		/** Very useful if you don't need want to setup a Skin Pack.
		@param stateValue
			Value in range [0; 2).
			0: Unchecked, 1: Checked
		@param skinName
			Name of the skin to use, i.e. m_manager->getSkinManager()->getSkins().find( skinName )

			If an empty name is set IdString(), then when the ToggleButton goes into this state,
			the skin won't be altered, meaning it will continue to use the skin it was using.
		@param forState
			The state to use, use special value States::NumStates to set this skin to all states
		*/
		void setSkin( uint8_t stateValue, Ogre::IdString skinName,
					  States::States forState = States::NumStates );

		/** Sets the current value of the ToggleButton. 0: Unchecked, 1: Checked
			If value is out of bounds, it gets clamped.
		@remarks
			Calling this function won't trigger the listeners, even if the value gets clamped.
			You can do that yourself by doing callActionListeners( Action::ValueChanged )
		*/
		void setCurrentValue( uint8_t currentValue );

		uint8_t getCurrentValue() const { return m_currentValue; }
		uint8_t getMaxValue() const { return 1u; }

		void notifyWidgetAction( Widget *widget, Action::Action action ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
