
#include "ColibriGui/ColibriRadarChart.h"

#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"

using namespace Colibri;

RadarChart::RadarChart( ColibriManager *manager ) :
	CustomShape( manager ),
	m_scale( 1.0f / std::numeric_limits<uint16_t>::max() )
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
size_t RadarChart::drawChartShape( size_t triOffset, enum ChartShapeType type,
								   Ogre::ColourValue shapeColor, Ogre::ColourValue backgroundColor,
								   float lineWidth, bool drawAsLine )
{
	const size_t numDataSeries = m_dataSeries.size();

	const float lineThickness = lineWidth;
	const Ogre::Real rot = 360.0f / ( (float)numDataSeries );

	const Ogre::Vector2 spoke( 0, -1 );

	const size_t num_shapes = ( drawAsLine ) ? 2u : 1u;

	for( size_t k = 0; k < num_shapes; ++k )
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
			current = next;
		}
	}
	return triOffset;
}
//-------------------------------------------------------------------------
void RadarChart::drawChartTriangles()
{
	// TODO: Remove hard coded example data for the chart if the series doesn't have values
	if( m_dataSeries.empty() )
	{
		std::vector<float> next_chart_coords = { 0.95f, 0.81f, 0.7f, 0.9f, 0.85f };
		std::vector<float> current_chart_coords = { 0.55f, 0.41f, 0.65f, 0.4f, 0.75f };
		for( size_t i = 0u; i < 5u; ++i )
		{
			DataEntry data;
			data.curr = current_chart_coords[i];
			data.next = next_chart_coords[i];
			m_dataSeries.push_back( data );
		}
	}

	// TODO: GET light and dark menu color from UIManager as well as alternating background color maybe
	// as param
	const Ogre::ColourValue backgroundColor =
		Ogre::ColourValue( 37.0 / 512.0, 42.0 / 512.0, 67.0 / 512.0, 1.0f );
	const Ogre::ColourValue darkColor = Ogre::ColourValue( 0.367f, 0.216f, 0.11f, 1.0f );
	const Ogre::ColourValue lightColor = Ogre::ColourValue( 1.0f, 0.588f, 0.11f, 1.0f );

	// TODO: Probably expose line width for Asi as param.
	const float lineWidth = 0.02f;
	drawRadarChart( backgroundColor, darkColor, lightColor, lineWidth );
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
void RadarChart::setDataSeries( const std::vector<DataEntry> &dataSeries,
								const LabelDisplay labelDisplay, const float fScale )
{
	const size_t numRequiredLabels = labelDisplay == LabelDisplayNone ? 0u : dataSeries.size();

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
	m_scale = fScale;

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
			if( labelDisplay == LabelDisplayNameOnly )
			{
				m_labels[i]->setText( m_dataSeries[i].name );
			}
			else
			{
				m_labels[i]->setText( m_dataSeries[i].name + "\n" +
									  std::to_string( m_dataSeries[i].curr * fScale ) );
			}

			m_labels[i]->sizeToFit();
		}
	}

	// TODO
	// this->setNumTriangles();
	// this->setTriangle();

	updateLabelsPosition();
}
//-------------------------------------------------------------------------
void RadarChart::setTransformDirty( uint32_t dirtyReason )
{
	// Only update if our size is directly being changed, not our parent's
	if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) == TransformDirtyScale )
	{
		updateLabelsPosition();
	}

	Widget::setTransformDirty( dirtyReason );
}
//-------------------------------------------------------------------------
void RadarChart::updateLabelsPosition()
{
	// TODO
}
