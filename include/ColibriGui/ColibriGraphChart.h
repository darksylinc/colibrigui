
#pragma once

#include "ColibriGui/ColibriCustomShape.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class RadarChart
		A widget used to draw a graph/chart.
	*/
	class GraphChart : public CustomShape
	{
	public:
		struct Column
		{
			/// Value for this data entry is in range (-inf; inf).
			/// See setDataRange().
			float *values;

			Colibri::Label *label;

			/// This dataset's colour lives in rectangle->setColour().
			/// But changes won't take effect until the next GraphChart::build call.
			Colibri::CustomShape *rectangle;
		};

		struct Params
		{
			uint16_t numLines;
			float    lineThickness;

			// In UV space, range [0; 1)
			Ogre::Vector2 graphInnerTopLeft;
			// In UV space, range (0; 1]
			Ogre::Vector2 graphInnerSize;

			Ogre::ColourValue lineColour;
			Ogre::ColourValue bgInnerColour;
			Ogre::ColourValue bgOuterColour;

			Params();
		};

	protected:
		std::vector<Column> m_columns;
		std::vector<float>  m_allValues;

		Ogre::TextureGpu *m_textureData;

		bool m_labelsDirty;

		bool m_autoMin;
		bool m_autoMax;
		bool m_markersOnRight;

		float m_autoMinMaxRounding;
		float m_minSample;
		float m_maxSample;
		float m_lastMinValue;
		float m_lastMaxValue;

		int32_t m_labelPrecision;

		std::vector<Colibri::Label *> m_markers;

		Params m_params;

		void positionGraphLegend();
		void positionMarkersInLines( const float minValue, const float maxValue );
		void positionLabels();

	public:
		GraphChart( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		/** Indicates the upper limits.
		@remarks
			All previous data may be cleared!
		@param numColumns
			Number of columns to display.
		@param entriesPerColumn
			Max number of elements per column.
			Max. value is 2048.
		*/
		void setMaxValues( uint32_t numColumns, uint32_t entriesPerColumn );

		/// Returns the setting set to setMaxValues().
		uint32_t getEntriesPerColumn() const;

		/** Sets the min & max values to display from our dataset.
		@param autoMin
			When true, we will auto-calculate the min value based on the minimum value
			we find on the dataset.
			<br/>
			The final value is:
				m_minValue = std::min( autoCalculatedMin, minValue );
		@param autoMax
			When true, we will auto-calculate the max value based on the maximum value
			we find on the dataset.
			<br/>
			The final value is:
				m_maxValue = std::max( autoCalculatedMax, maxValue );
		@param minValue
			Minimum value to display. Values below this threshold are not shown unless autoMin = true.
		@param maxValue
			Maximum value to display. Values above this threshold are not shown unless autoMax = true.
		@param autoMinMaxRounding
			When autoMin or autoMax are true; this value control the steps in which to round to.
			For example if this graph is evaluating FPS (frames per second) with VSync on, many samples
			will be in range [58; 63]. With this setting set to 0 (disabled), maxValue will be constantly
			jumping.
			By setting autoMinMaxRounding = 10.0f, the max value will be rounded to 50, 60, or 70
			depending on the input data.
		*/
		void setDataRange( bool autoMin, bool autoMax, float minValue, float maxValue,
						   float autoMinMaxRounding );

		/// Direct access to data to modify it.
		/// Call syncChart() once you're done modifying it.
		std::vector<GraphChart::Column> &getColumns() { return m_columns; }

		const std::vector<GraphChart::Column> &getColumns() const { return m_columns; }

		const GraphChart::Params &getParams() const { return m_params; }

		/** Sets the font size of the markers.
		@remarks
			If called, must be done so after build().
			Calling build() again may cause us to forget this setting.
		@param fontSize
			Font size.
		*/
		void setMarkersFontSize( FontSize fontSize );

		/** Defines whether the markers are displayed on the left or right.
		@param bOnRight
			True to display on the right, false on the left.
		*/
		void setMarkersOnRight( bool bOnRight )
		{
			m_markersOnRight = bOnRight;
			m_labelsDirty = true;
		}
		bool getMarkersOnRight() const { return m_markersOnRight; }

		/** Set how many decimals to show on the labels.
		@param precision
			Negative value for default.
		*/
		void setLabelsPrecision( int32_t precision )
		{
			m_labelPrecision = precision;
			m_labelsDirty = true;
		}
		int getLabelsPrecision() const { return m_labelPrecision; }

		/** MUST be called after setMaxValues() and before rendering.
			Per-column must be set before calling this function via:
			@code
				graphChart->getColumns()[i].rectangle->
					setColour( true, Ogre::ColourValue( 0.0f, 1.0f, 0.0f, 0.85f ) );
			@endcode
		@remarks
			Changing these settings can trigger a shader recompilation.
			You *can* call syncChart() after build().
			<br/>
			You *can* call this function again if you wish to change settings.
		@param params
		*/
		void build( const Params &params );

		/// After you've called build() you can still modify all the values of Column::values.
		/// Every time you do that, you must call syncChart() again.
		void syncChart();

		void setTransformDirty( uint32_t dirtyReason ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
