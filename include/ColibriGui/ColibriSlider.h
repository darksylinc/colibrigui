
#pragma once

#include "ColibriGui/ColibriWidget.h"

#include "OgreId.h"
#include "OgreIdString.h"

namespace Ogre
{
	class HlmsUnlitDatablock;
}

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class Slider
		Displays an interactive slider widget using two Renderables

		Layer 0 is drawn behind layer 1
	*/
	class Slider colibri_final : public Widget, Ogre::IdObject
	{
	protected:
		Renderable *colibrigui_nullable m_layers[2];

		int32_t m_currentValue;

		int32_t m_minValue;
		int32_t m_maxValue;
		int32_t m_denominator;

		float         m_cursorOffset;
		float         m_lineSize;
		Ogre::Vector2 m_handleProportion;
		bool          m_vertical;
		bool          m_alwaysInside;

	protected:
		void updateSlider();

		void processCursorPosition( const Ogre::Vector2 &pos, bool cursorBegin = false );

	public:
		Slider( ColibriManager *manager );

		virtual void _initialize();
		virtual void _destroy();

		/// Sets a different skin pack (than default) for line and/or handle
		///
		/// Leave an empty Ogre::IdString() if you wish to retain the previous skin pack,
		/// i.e. only changing one of them
		void setSkinPack( Ogre::IdString linePackName, Ogre::IdString handlePackName );

		/// @copydoc Renderable::setVisualsEnabled
		void         setVisualsEnabled( bool bEnabled );
		virtual bool isVisualsEnabled() const colibri_final;

		virtual void setState( States::States state, bool smartHighlight = true,
							   bool broadcastEnable = false );

		/** Sets the size of the handle relative to the full
			width (m_vertical = true) or height (m_vertical = false) of the Slider.

			And sets the size of the handle, fixed in virtual canvas units

		@param handleProportion
			Should be in range [0; 1].
			Values outside that range are accepted but may cause visual artifacts
			If both .x and .y are the same value, then the handle will be square
		@param lineSize
			Should be in range [0; inf)
			In virtual canvas units
		*/
		void setElementsSize( const Ogre::Vector2 &handleProportion, const float lineSize );
		const Ogre::Vector2 &getHandleProportion() const { return m_handleProportion; }
		float                getLineSize() const { return m_lineSize; }

		/** Sets the current value of the slider. If the value is outside of range,
			it gets clamped to range [m_minValue; m_maxValue]
			See Slider::setRange
		@remarks
			Calling this function won't trigger the listeners, even if the value gets clamped.
			You can do that yourself by doing callActionListeners( Action::ValueChanged )
		*/
		void setCurrentValue( int32_t currentValue );

		/** Sets the denominator in order to be able to represent fractional values.
			for example calling:
			@code
				slider->setCurrentValue( 5 );
				slider->setDenominator( 2 );
			@endcode
			Will make the Slider::getCurrentValueProcessed return 2.5f since 5 / 2 = 2.5
		@remarks
			See Slider::setCurrentValue remarks
		*/
		void setDenominator( int32_t denominator );

		/// Returns the current value
		int32_t getCurrentValueRaw() const { return m_currentValue; }
		int32_t getDenominator() const { return m_denominator; }
		/// Returns m_currentValue / m_denominator
		float getCurrentValueProcessed() const;
		/// Returns value in range [0.0f; 1.0f]
		float getCurrentValueUnorm() const;

		/** Sets the minimum & maximum range of integers the slider can go to.
			If m_currentValue is currently outside the range, it will be clamped
		@remarks
			See Slider::setCurrentValue remarks
		@param minValue
			Must be minValue < maxValue
		@param maxValue
			Must be minValue < maxValue
		*/
		void setRange( int32_t minValue, int32_t maxValue );

		int32_t getMinValue() const { return m_minValue; }
		int32_t getMaxValue() const { return m_maxValue; }

		/// When false, the handle reaches half outside when at 0% and 100%
		/// When true, the handle is always contained inside the background
		///
		/// e.g. at 0%
		///
		///	@code
		/// m_alwaysInside = false:
		///
		///		  |                  |
		///     -----                |
		///		| |-|----------------|
		///		-----                |
		///		  |                  |
		///
		/// m_alwaysInside = true:
		///
		///		  |                  |
		///       -----              |
		///		  |---|--------------|
		///		  -----              |
		///		  |                  |
		/// @endcode
		void setAlwaysInside( bool bAlwaysInside );
		bool getAlwaysInside() const { return m_alwaysInside; }

		void setVertical( bool bVertical );
		bool getVertical() const { return m_vertical; }

		Renderable *getSliderLine();
		Renderable *getSliderHandle();

		virtual void setTransformDirty( uint32_t dirtyReason ) colibri_final;

		virtual void notifyCursorMoved( const Ogre::Vector2 &posNDC );
		virtual void _notifyActionKeyMovement( Borders::Borders direction );
	};
}  // namespace Colibri

COLIBRIGUI_ASSUME_NONNULL_END
