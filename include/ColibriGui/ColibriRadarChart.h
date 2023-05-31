
#pragma once

#include "ColibriGui/ColibriCustomShape.h"

COLIBRI_ASSUME_NONNULL_BEGIN

namespace Colibri
{
	/** @ingroup Controls
	@class RadarChart
		A widget used to draw a radar chart, aka Spider Graph / Spider Chart.

		Thick lines are drawn using a simple trick: If we have e.g. 6 datapoints,
		then we draw an hexagon and then a smaller hexagon on top with the same colour
		as the background colour.

		Thus a smaller hexagon visually results in a thicker line.
	*/
	class RadarChart : public CustomShape
	{
	public:
		struct DataEntry
		{
			/// Value for this data entry in range [0; 1]
			float curr;

			/// Value for the next "updgrade".
			///
			/// It's best explained with an example: Let's say an RPG Character
			/// you have STR, HP, MP, DEX stats. These can be upgraded by spending points.
			///
			/// With 'next' you can show two superimposed graphs: one indicates the current STR value,
			/// the other indicates the next value to acquire if the user spends points on the
			/// next STR upgrade.
			float next;

			/// Label for this data entry
			std::string name;
		};

		enum LabelDisplay
		{
			LabelDisplayNone,
			LabelDisplayName
		};

		struct DisplaySettings
		{
			/// How big the chart should be. In range (0; 1)
			float chartSize;

			/// The distance from the chart to the labels.
			/// In range [0; 1] but for best results make sure:
			///
			/// chartSize + chartToLabelDistance <= 1
			float chartToLabelDistance;

			Ogre::ColourValue emptyBackgroundColor;
			Ogre::ColourValue radarBackgroundColor;
			Ogre::ColourValue darkColor;
			Ogre::ColourValue lightColor;
			Ogre::ColourValue fgLineColor;

			float fgLineThickness;

			float    bgTopRingThickness;
			float    bgRingThickness;
			float    bgSpokeThickness;
			uint32_t numRings;

			DisplaySettings();
		};

	protected:
		std::vector<DataEntry>        m_dataSeries;
		std::vector<Colibri::Label *> m_labels;

		/// Whether to show text on screen and how
		LabelDisplay m_labelDisplay;

		DisplaySettings m_displaySettings;

		Ogre::Vector2 m_proportion;

		/// Labels are in virtual canvas, which means we have to reposition
		/// them every time our widget changes its shape's size
		void updateLabelsPosition();

		Ogre::Vector2 rotate( Ogre::Vector2 start, Ogre::Radian angle );

		/// Draws the polygon which can be a pentagon/hexagon/etc depending on the number of data entries
		size_t drawChartShape( size_t triOffset, Ogre::ColourValue shapeColor,
							   Ogre::ColourValue lineColor, float lineThickness, bool bDrawCurr );

		size_t drawBackgroundShape( size_t triOffset, const Ogre::ColourValue darkColor,
									const Ogre::ColourValue backgroundColor );

		/// Draws the entire chart
		void drawRadarChart();

	public:
		RadarChart( ColibriManager *manager );

		void _initialize() override;
		void _destroy() override;

		/** Set the display settings.
		@remarks
			If you've already called setDataSeries(), you must call
			redrawRadarChart() for changes to take effect.
		@param displaySettings
			New display settings.
		*/
		void setDisplaySettings( const DisplaySettings &displaySettings );

		const DisplaySettings &getDisplaySettings() const { return m_displaySettings; }

		/** Sets the proportion the chart should occupy. Useful if you want the background
			to expand to be much bigger or the widget does not have an aspect ratio of 1:1.

			It affects the text as well.
		@remarks
			If you've already called setDataSeries(), you must call
			redrawRadarChart() for changes to take effect.
		@param proportion
			Value in range [0; 1]
		*/
		void setPropotion( const Ogre::Vector2 &proportion );

		const Ogre::Vector2 &getPropotion() const { return m_proportion; }

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
							LabelDisplay                  labelDisplay = LabelDisplayNone );

		/// Users can directly access the labels to modify them
		/// (i.e. change the text or its formatting).
		///
		/// The Labels are only changed when setDataSeries() is called.
		/// The labels' size is important for positioning.
		///
		/// The labels' position is changed every time redrawRadarChart() is called though.
		/// If you've modified the labels' position, make sure to call redrawRadarChart()
		/// after you're done in order for those changes to take effect.
		const std::vector<Colibri::Label *> &getLabels() const { return m_labels; }

		/// Redraws the radar triangles to apply the latest display settings
		/// and repositions the labels.
		void redrawRadarChart();

		void setTransformDirty( uint32_t dirtyReason ) override;
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
