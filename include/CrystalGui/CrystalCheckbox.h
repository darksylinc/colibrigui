
#pragma once

#include "CrystalGui/CrystalWidget.h"

#include "OgreIdString.h"

CRYSTALGUI_ASSUME_NONNULL_BEGIN

namespace Crystal
{
	/** @ingroup Controls
	@class Checkbox
	*/
	class Checkbox : public Widget, public WidgetActionListener
	{
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

		Button *m_button;
		Button *m_tickmark;

		uint8_t	m_currentValue;
		bool	m_triState;
		HorizWidgetDir::HorizWidgetDir	m_horizDir;
		Mode	m_mode;

		Ogre::Vector2	m_valueLocationFraction;
		float m_tickmarkHeightFractionSize;
		float m_marginHeightFractionSize;

		/// Each skin pack per m_skinPacks[m_currentValue]
		SkinInfo const * crystalgui_nullable m_skinPacks[3][States::NumStates];

		void updateTickmark();

	public:
		Checkbox( CrystalManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		void setSkinPack( Ogre::IdString skinPackName );

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

		uint8_t getMaxValue() const						{ return m_triState ? 2u : 1u; }

		virtual void setTransformDirty();

		virtual void notifyWidgetAction( Widget *widget, Action::Action action );
	};
}

CRYSTALGUI_ASSUME_NONNULL_END
