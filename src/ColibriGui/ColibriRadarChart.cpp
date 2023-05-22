
#include "ColibriGui/ColibriRadarChart.h"

#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"

using namespace Colibri;

RadarChart::DisplaySettings::DisplaySettings() :
	chartSize( 0.7f ),
	chartToLabelDistance( 0.15f ),
	backgroundColor( 37.0 / 512.0, 42.0 / 512.0, 67.0 / 512.0, 1.0f ),
	darkColor( 0.367f, 0.216f, 0.11f, 1.0f ),
	lightColor( 1.0f, 0.588f, 0.11f, 1.0f ),
	lineThickness( 0.02f )
{
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
RadarChart::RadarChart( ColibriManager *manager ) :
	CustomShape( manager ),
	m_labelDisplay( LabelDisplayNone )
{
}
//-------------------------------------------------------------------------
void RadarChart::_initialize()
{
	CustomShape::_initialize();
	_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::RadarChart ) );
}
//-------------------------------------------------------------------------
void RadarChart::_destroy()
{
	// No need to destroy m_labels. They are our children and will be destroyed
	// automatically with us.
	CustomShape::_destroy();
}
//-------------------------------------------------------------------------
Ogre::Vector2 RadarChart::rotate( Ogre::Vector2 start, Ogre::Real angle )
{
	Ogre::Vector4 orientation;
	const Ogre::Real rot = Ogre::Degree( angle ).valueRadians();
	orientation.x = std::cos( rot );
	orientation.z = std::sin( rot );
	orientation.y = -orientation.z;
	orientation.w = orientation.x;
	return Widget::mul( orientation, start );
}
//-------------------------------------------------------------------------
size_t RadarChart::drawChartShape( size_t triOffset, ChartShapeType type, Ogre::ColourValue shapeColor,
								   Ogre::ColourValue backgroundColor, float lineWidth, bool drawAsLine )
{
	const size_t numDataSeries = m_dataSeries.size();

	const float lineThickness = lineWidth;
	const Ogre::Real rot = 360.0f / ( (float)numDataSeries );

	const Ogre::Vector2 spoke( 0, -m_displaySettings.chartSize );

	const size_t numShapes = ( drawAsLine ) ? 2u : 1u;

	for( size_t k = 0; k < numShapes; ++k )
	{
		for( size_t i = 0; i < numDataSeries; ++i )
		{
			float value = 1.0f;
			float nextValue = 1.0f;
			Ogre::Vector2 current = spoke;
			current.y += lineThickness * static_cast<Ogre::Real>( k );
			if( type != ShapeOutline )
				value = ( type == ShapeCurrent ) ? m_dataSeries[i].curr : m_dataSeries[i].next;
			current *= value;

			const size_t nextIdx = ( i + 1u < numDataSeries ) ? ( i + 1u ) : 0u;
			Ogre::Vector2 next = spoke;
			next.y += lineThickness * static_cast<Ogre::Real>( k );
			if( type != ShapeOutline )
				nextValue =
					( type == ShapeCurrent ) ? m_dataSeries[nextIdx].curr : m_dataSeries[nextIdx].next;
			next *= nextValue;

			Ogre::ColourValue color = ( k == 0 ) ? shapeColor : backgroundColor;

			setTriangle( triOffset * 3u, Ogre::Vector2( 0, 0 ),
						 rotate( current, rot * static_cast<Ogre::Real>( i ) ),
						 rotate( next, rot * static_cast<Ogre::Real>( nextIdx ) ), color );
			triOffset++;
		}
	}
	return triOffset;
}
//-------------------------------------------------------------------------
void RadarChart::drawRadarChart( const Ogre::ColourValue backgroundColor,
								 const Ogre::ColourValue darkColor, const Ogre::ColourValue lightColor,
								 const float lineWidth )
{
	// 2x for the outline, 2x for the line chart for next values, 1x for solid chart for curr values, +
	// 2u for the background quad
	const size_t numTriangles = m_dataSeries.size() * 5u + 2u;
	setNumTriangles( numTriangles );

	size_t triOffset = 0u;

	// Draw the background
	setTriangle( triOffset * 3u, Ogre::Vector2( -1, -1 ), Ogre::Vector2( 1, 1 ), Ogre::Vector2( 1, -1 ),
				 backgroundColor );
	triOffset++;
	setTriangle( triOffset * 3u, Ogre::Vector2( 1, 1 ), Ogre::Vector2( -1, -1 ), Ogre::Vector2( -1, 1 ),
				 backgroundColor );
	triOffset++;

	// Draw the charts
	triOffset = drawChartShape( triOffset, ShapeOutline, darkColor, backgroundColor, lineWidth, true );
	triOffset = drawChartShape( triOffset, ShapeNext, lightColor, darkColor, lineWidth, true );

	// TODO: Maybe draw spokes for next shape (line shape). Old charts had spokes, not sure how to draw
	// those here, we'd need draw lines from the origin to each next coordinate matching the thickness of
	// lineWidth.

	triOffset = drawChartShape( triOffset, ShapeCurrent, lightColor, backgroundColor, lineWidth, false );
}
//-------------------------------------------------------------------------
void RadarChart::drawChartTriangles()
{
	drawRadarChart( m_displaySettings.backgroundColor, m_displaySettings.darkColor,
					m_displaySettings.lightColor, m_displaySettings.lineThickness );
}
//-------------------------------------------------------------------------
void RadarChart::setDisplaySettings( const DisplaySettings &displaySettings )
{
	m_displaySettings = displaySettings;
}
//-------------------------------------------------------------------------
void RadarChart::setDataSeries( const std::vector<DataEntry> &dataSeries,
								const LabelDisplay labelDisplay )
{
	const size_t numRequiredLabels = labelDisplay == LabelDisplayNone ? 0u : dataSeries.size();
	m_labelDisplay = labelDisplay;

	{
		// Remove excess labels
		const size_t oldNumLabels = m_labels.size();
		const size_t numLabelsToRemove = std::max( oldNumLabels, numRequiredLabels ) - numRequiredLabels;

		// Remove the last labels in the container
		const ptrdiff_t toRemoveStart = ptrdiff_t( oldNumLabels - numLabelsToRemove );
		std::vector<Label *>::const_iterator itor = m_labels.begin() + toRemoveStart;
		std::vector<Label *>::const_iterator endt = m_labels.end();

		while( itor != endt )
		{
			m_manager->destroyWidget( *itor );
			++itor;
		}

		m_labels.erase( m_labels.begin() + toRemoveStart, m_labels.end() );
	}

	m_dataSeries = dataSeries;

	// Create required labels
	const size_t numLabelsToAdd = std::max( m_labels.size(), numRequiredLabels ) - m_labels.size();

	if( numLabelsToAdd > 0u )
	{
		m_labels.reserve( numRequiredLabels );

		for( size_t i = 0u; i < numLabelsToAdd; ++i )
		{
			Label *newLabel = m_manager->createWidget<Label>( this );
			newLabel->setTextHorizAlignment( TextHorizAlignment::Center );
			m_labels.emplace_back( newLabel );
		}
	}

	if( labelDisplay != LabelDisplayNone )
	{
		const size_t numDataSeries = dataSeries.size();

		for( size_t i = 0u; i < numDataSeries; ++i )
		{
			m_labels[i]->setText( m_dataSeries[i].name );
			m_labels[i]->sizeToFit();
		}
	}

	redrawRadarChart();
}
//-------------------------------------------------------------------------
void RadarChart::redrawRadarChart()
{
	drawChartTriangles();
	updateLabelsPosition();
}
//-------------------------------------------------------------------------
void RadarChart::setTransformDirty( uint32_t dirtyReason )
{
	// Only update if our size is directly being changed, not our parent's
	if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) == TransformDirtyScale )
		updateLabelsPosition();

	Widget::setTransformDirty( dirtyReason );
}
//-------------------------------------------------------------------------
void RadarChart::updateLabelsPosition()
{
	if( m_labelDisplay == LabelDisplayNone )
		return;

	const size_t numDataSeries = m_dataSeries.size();

	COLIBRI_ASSERT_LOW( numDataSeries == m_labels.size() );

	const Ogre::Real rot = 360.0f / ( (float)numDataSeries );

	const Ogre::Vector2 spoke(
		0, -( m_displaySettings.chartSize + m_displaySettings.chartToLabelDistance ) );

	for( size_t i = 0; i < numDataSeries; ++i )
	{
		const Ogre::Vector2 ndcPos = rotate( spoke, rot * static_cast<Ogre::Real>( i ) );
		Ogre::Vector2 canvasPos = ( ndcPos * 0.5f + 0.5f ) * m_size;

		canvasPos = canvasPos - m_labels[i]->getSize() * 0.5f;
		canvasPos.makeCeil( Ogre::Vector2::ZERO );
		canvasPos.makeFloor( m_size - m_labels[i]->getSize() );

		m_labels[i]->setTopLeft( canvasPos );
	}
}
