
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgreIdString.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Checkbox
	*/
	class Checkbox : public Widget, public WidgetActionListener
	{
	public:
		enum Mode
		{
			/** Layout is as follows:
					[x] [button]
				or:
					[button] [x]
			*/
			TickButton,
			/** Layout is as follows:
					[ [x] button ]
				or:
					[ button [x] ]
			*/
			BigButton
		};

		enum StateMode
		{
			/// The checkbox behaves like a button. There's only 1 state:
			/// 0: Unchecked
			///
			/// Useful when a button must contain an icon inside, so instead of a Button,
			/// you can just use NoState Checkbox
			NoState,

			/// The default checkbox behavior:
			/// 0: Unchecked, 1: Checked
			TwoState,

			/// The checkbox cycles between 3 states:
			/// 0: Unchecked, 1: Checked, 2: Tri-state checked
			TriState,
		};

	protected:
		Button *m_button;
		Button *m_tickmark;

		uint8_t   m_currentValue;
		StateMode m_stateMode;

		HorizWidgetDir::HorizWidgetDir m_horizDir;
		Mode                           m_mode;

		float         m_tickmarkMargin;
		Ogre::Vector2 m_tickmarkSize;

		/// Each skin pack per m_skinPacks[m_currentValue]
		SkinInfo const *colibri_nullable m_skinPacks[3][States::NumStates];

		void updateTickmark();

	public:
		Checkbox( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		Button *getButton() { return m_button; }

		/// Use with care. Modifying m_tickmark too much may break how the Checkbox works.
		Button *getTickmark() { return m_tickmark; }

		void setSkinPack( Ogre::IdString skinPackName );

		/** Sets the skin packs for each state (unchecked, checked, tri-checked)
		@param stateValue
			Value in range [0; 3).
			0: Unchecked, 1: Checked, 2: Tri-state checked
		@param skinPackName
			Skin pack to set.

			If an empty name is set IdString(), then when the checkbox goes into this state,
			the skin won't be altered, meaning it will continue to use the skin it was using.
		*/
		void setTickmarkSkinPack( uint8_t stateValue, Ogre::IdString skinPackName );

		/** Same as calling m_tickmark->setSkin()
			Very useful if you don't need want to setup a Skin Pack.
		@param stateValue
			Value in range [0; 3).
			0: Unchecked, 1: Checked, 2: Tri-state checked
		@param skinName
			Name of the skin to use, i.e. m_manager->getSkinManager()->getSkins().find( skinName )

			If an empty name is set IdString(), then when the checkbox goes into this state,
			the skin won't be altered, meaning it will continue to use the skin it was using.
		@param forState
			The state to use, use special value States::NumStates to set this skin to all states
		*/
		void setTickmarkSkin( uint8_t stateValue, Ogre::IdString skinName,
							  States::States forState = States::NumStates );

		void setCheckboxMode( Mode mode );
		Mode getCheckboxMode() const { return m_mode; }

		void          setTickmarkMarginAndSize( float margin, const Ogre::Vector2 &size );
		float         getTickmarkMargin() const { return m_tickmarkMargin; }
		Ogre::Vector2 getTickmarkSize() const { return m_tickmarkSize; }

		void setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir );
		HorizWidgetDir::HorizWidgetDir getHorizWidgetDir() const { return m_horizDir; }

		/** Sets the current value of the Checkbox. 0: Unchecked, 1: Checked, 2: Tri-state checked
			If value is out of bounds, it gets clamped.
		@remarks
			Calling this function won't trigger the listeners, even if the value gets clamped.
			You can do that yourself by doing callActionListeners( Action::ValueChanged )
		*/
		void setCurrentValue( uint8_t currentValue );

		/// Deprecated. Use setStateMode() instead.
		/// Setting this value to false is the same as calling setStateMode( TwoState )
		COLIBRI_DEPRECATED
		void setTriState( bool triState );

		/// Deprecated. Use getStateMode() instead.
		COLIBRI_DEPRECATED
		bool getTriState() const { return m_stateMode == TriState; }

		/// Sets the current state mode. See StateMode
		void setStateMode( StateMode stateMode );

		StateMode getStateMode() const { return m_stateMode; }

		uint8_t getCurrentValue() const { return m_currentValue; }
		uint8_t getMaxValue() const { return static_cast<uint8_t>( m_stateMode ); }

		/// @copydoc Label::sizeToFit
		void sizeToFit( float maxAllowedWidth = std::numeric_limits<float>::max(),
						TextHorizAlignment::TextHorizAlignment newHorizPos = TextHorizAlignment::Left,
						TextVertAlignment::TextVertAlignment   newVertPos = TextVertAlignment::Top,
						States::States                         baseState = States::NumStates );

		void setTransformDirty( uint32_t dirtyReason ) final;

		void notifyWidgetAction( Widget *widget, Action::Action action ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
