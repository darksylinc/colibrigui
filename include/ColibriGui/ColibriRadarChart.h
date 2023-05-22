
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
			LabelDisplayNameOnly,
			LabelDisplayNameAndValue
		};

		enum ChartShapeType
		{
			ShapeOutline,
			ShapeCurrent,
			ShapeNext
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

		Ogre::Vector2 rotate( Ogre::Vector2 start, Ogre::Real angle );

		void drawRadarChart( Ogre::ColourValue backgroundColor, Ogre::ColourValue darkColor,
							 Ogre::ColourValue lightColor, float lineWidth );

		size_t drawChartShape( size_t tri_offset, enum ChartShapeType type, Ogre::ColourValue shapeColor,
							   Ogre::ColourValue backgroundColor, float lineWidth, bool drawAsLine );

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
		void drawChartTriangles();
	};
}  // namespace Colibri

COLIBRI_ASSUME_NONNULL_END
