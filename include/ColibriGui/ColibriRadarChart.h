
#pragma once

#include "ColibriGui/ColibriCustomShape.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class RadarChart
		A widget used to draw a radar chart, aka Spider Graph / Spider Chart.
	*/
	class RadarChart : public CustomShape
	{
	public:
		struct DataEntry
		{
			/// Value for this data entry, where 0 is minimum value
			/// and 0xFFFF the maximum possible value.
			///
			/// The displayed value (if shown on screen) will be scaled
			/// by RadarChart::m_scale
			uint16_t value;
			/// Label for this data entry
			std::string name;
		};

		enum LabelDisplay
		{
			LabelDisplayNone,
			LabelDisplayNameOnly,
			LabelDisplayNameAndValue
		};

	protected:
		std::vector<DataEntry>        m_dataSeries;
		std::vector<Colibri::Label *> m_labels;

		/// Scale
		float m_scale;

		/// Whether to show text on screen and how
		LabelDisplay m_labelDisplay;

		/// Labels are in virtual canvas, which means we have to reposition
		/// them every time our widget changes its shape's size
		void updateLabelsPosition();

	public:
		RadarChart( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		/** Replaces the current data series with a new one.
		@param dataSeries
			Data series to display.
		@param labelDisplay
			How to display the label, if any.
		@param fScale
			Data series are in integers. This scale is only used
			when displaying the values in text on screen
		*/
		void setDataSeries( const std::vector<DataEntry> &dataSeries,
							LabelDisplay                  labelDisplay = LabelDisplayNone,
							float                         fScale = 1.0f / 65535.0f );

		void setTransformDirty( uint32_t dirtyReason ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
