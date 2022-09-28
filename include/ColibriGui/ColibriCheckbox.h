
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

	protected:
		Button *m_button;
		Button *m_tickmark;

		uint8_t	m_currentValue;
		bool	m_triState;
		HorizWidgetDir::HorizWidgetDir	m_horizDir;
		Mode	m_mode;

		float			m_tickmarkMargin;
		Ogre::Vector2	m_tickmarkSize;

		/// Each skin pack per m_skinPacks[m_currentValue]
		SkinInfo const * colibri_nullable m_skinPacks[3][States::NumStates];

		void updateTickmark();

	public:
		Checkbox( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		Button* getButton()								{ return m_button; }

		void setSkinPack( Ogre::IdString skinPackName );

		void setCheckboxMode( Mode mode );
		Mode getCheckboxMode() const					{ return m_mode; }

		void setTickmarkMarginAndSize( float margin, const Ogre::Vector2 &size );
		float getTickmarkMargin() const					{ return m_tickmarkMargin; }
		Ogre::Vector2 getTickmarkSize() const			{ return m_tickmarkSize; }

		void setHorizWidgetDir( HorizWidgetDir::HorizWidgetDir horizWidgetDir );
		HorizWidgetDir::HorizWidgetDir getHorizWidgetDir() const	{ return m_horizDir; }

		/** Sets the current value of the Checkbox. 0: Unchecked, 1: Checked, 2: Tri-state checked
			If value is out of bounds, it gets clamped.
		@remarks
			Calling this function won't trigger the listeners, even if the value gets clamped.
			You can do that yourself by doing callActionListeners( Action::ValueChanged )
		*/
		void setCurrentValue( uint8_t currentValue );

		void setTriState( bool triState );
		bool getTriState() const						{ return m_triState; }

		uint8_t getCurrentValue() const 				{ return m_currentValue; }
		uint8_t getMaxValue() const						{ return m_triState ? 2u : 1u; }

		/// @copydoc Label::sizeToFit
		void sizeToFit( float maxAllowedWidth=std::numeric_limits<float>::max(),
						TextHorizAlignment::TextHorizAlignment newHorizPos=TextHorizAlignment::Left,
						TextVertAlignment::TextVertAlignment newVertPos=TextVertAlignment::Top,
						States::States baseState=States::NumStates );

		void setTransformDirty( uint32_t dirtyReason ) final;

		void notifyWidgetAction( Widget *widget, Action::Action action ) override;
	};
}

COLIBRI_ASSUME_NONNULL_END
