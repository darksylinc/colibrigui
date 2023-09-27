
#pragma once

#include "ColibriGui/ColibriCustomShape.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class RadarChart
		A widget used to draw a graph/chart.
	@remarks
		For various technical reasons, the number of datapoints that can be displayed
		per column is 2048.
	*/
	class GraphChart : public CustomShape
	{
	public:
		struct Column
		{
			/// Value for this data entry in range [0; 65535]
			uint16_t *values;

			// void setValue( const size_t idx, const float value ) { values[idx] = value * 65535.0f; }
		};

	protected:
		std::vector<Column>   m_columns;
		std::vector<uint16_t> m_allValues;

		Ogre::TextureGpu *m_textureData;

	public:
		GraphChart( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		/** Indicates the upper limits.
		@param numColumns
			Number of columns to display.
		@param entriesPerColumn
			Max number of elements per column.
			Max. value is 2048.
		*/
		void setMaxValues( uint32_t numColumns, uint32_t entriesPerColumn );

		uint32_t getEntriesPerColumn() const;

		/// Direct access to data to modify it.
		/// Call syncChart() once you're done modifying it.
		std::vector<Column> &getColumns() { return m_columns; }

		const std::vector<Column> &getColumns() const { return m_columns; }

		void syncChart();
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
