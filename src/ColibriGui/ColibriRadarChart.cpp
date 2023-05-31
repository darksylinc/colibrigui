
#include "ColibriGui/ColibriRadarChart.h"

#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"

using namespace Colibri;

// #define USE_PENTAGONS

RadarChart::DisplaySettings::DisplaySettings() :
	chartSize( 0.7f ),
	chartToLabelDistance( 0.15f ),
	emptyBackgroundColor( 37.0 / 512.0, 42.0 / 512.0, 67.0 / 512.0, 1.0f ),
	radarBackgroundColor( 37.0 / 512.0, 42.0 / 512.0, 67.0 / 512.0, 1.0f ),
	darkColor( 0.367f, 0.216f, 0.11f, 1.0f ),
	lightColor( 1.0f, 0.588f, 0.11f, 1.0f ),
	fgLineColor( 1.0f, 1.0f, 1.0f ),
	fgLineThickness( 0.02f ),
	bgTopRingThickness( 0.04f ),
	bgRingThickness( 0.02f ),
	bgSpokeThickness( 0.02f ),
	numRings( 5u )
{
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
RadarChart::RadarChart( ColibriManager *manager ) :
	CustomShape( manager ),
	m_labelDisplay( LabelDisplayNone ),
	m_proportion( Ogre::Vector2::UNIT_SCALE )
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
Ogre::Vector2 RadarChart::rotate( Ogre::Vector2 start, Ogre::Radian angle )
{
	Ogre::Vector4 orientation;
	const Ogre::Real rot = angle.valueRadians();
	orientation.x = std::cos( rot );
	orientation.z = std::sin( rot );
	orientation.y = -orientation.z;
	orientation.w = orientation.x;
	return Widget::mul( orientation, start );
}
//-------------------------------------------------------------------------
size_t RadarChart::drawChartShape( size_t triOffset, const Ogre::ColourValue shapeColor,
								   const Ogre::ColourValue lineColor, const float lineThickness,
								   const bool bDrawCurr )
{
	const size_t numDataSeries = m_dataSeries.size();

	const Ogre::Degree rot( Ogre::Real( 360.0f ) / Ogre::Real( numDataSeries ) );

	const Ogre::Vector2 spoke( 0, -m_displaySettings.chartSize );
	const Ogre::Vector2 proportion = m_proportion;

	for( size_t i = 0; i < numDataSeries; ++i )
	{
		float value = 1.0f;

		Ogre::Vector2 current = spoke;
		value = bDrawCurr ? m_dataSeries[i].curr : m_dataSeries[i].next;
		current *= value;

		const size_t nextIdx = ( i + 1u < numDataSeries ) ? ( i + 1u ) : 0u;
		Ogre::Vector2 next = spoke;
		value = bDrawCurr ? m_dataSeries[nextIdx].curr : m_dataSeries[nextIdx].next;
		next *= value;

		setTriangle( triOffset * 3u, Ogre::Vector2( 0, 0 ) * proportion,
					 rotate( current, rot * static_cast<Ogre::Real>( i ) ) * proportion,
					 rotate( next, rot * static_cast<Ogre::Real>( nextIdx ) ) * proportion, lineColor );
		triOffset++;
	}

#ifdef USE_PENTAGONS
	const float pentagonTop = 1.0f - lineThickness * 1.5f;
	const float pentagonBottom = lineThickness;

	const Ogre::Radian rotOffsetTop( 0.5f * lineThickness / pentagonTop );
	const Ogre::Radian rotOffsetBottom( 0.5f * lineThickness / pentagonBottom );

	for( size_t i = 0; i < numDataSeries; ++i )
	{
		const size_t nextIdx = ( i + 1u < numDataSeries ) ? ( i + 1u ) : 0u;

		Ogre::Vector2 currRingTop = spoke;
		currRingTop.y *= pentagonTop * m_dataSeries[i].curr;

		Ogre::Vector2 currRingBottom = spoke;
		currRingBottom.y *= pentagonBottom * m_dataSeries[i].curr;

		Ogre::Vector2 nextRingTop = spoke;
		nextRingTop.y *= pentagonTop * m_dataSeries[nextIdx].curr;

		Ogre::Vector2 nextRingBottom = spoke;
		nextRingBottom.y *= pentagonBottom * m_dataSeries[nextIdx].curr;

		setTriangle(
			triOffset * 3u,  //
			rotate( currRingTop, rot * static_cast<Ogre::Real>( i ) + rotOffsetTop ) * proportion,
			rotate( currRingBottom, rot * static_cast<Ogre::Real>( i ) + rotOffsetBottom ) * proportion,
			rotate( nextRingTop, rot * static_cast<Ogre::Real>( nextIdx ) - rotOffsetTop ) * proportion,
			shapeColor );
		triOffset++;

		setTriangle(
			triOffset * 3u,  //
			rotate( nextRingTop, rot * static_cast<Ogre::Real>( nextIdx ) - rotOffsetTop ) * proportion,
			rotate( currRingBottom, rot * static_cast<Ogre::Real>( i ) + rotOffsetBottom ) * proportion,
			rotate( nextRingBottom, rot * static_cast<Ogre::Real>( nextIdx ) - rotOffsetBottom ) *
				proportion,
			shapeColor );
		triOffset++;
	}
#else
	for( size_t i = 0; i < numDataSeries; ++i )
	{
		Ogre::Vector2 current = spoke * ( bDrawCurr ? m_dataSeries[i].curr : m_dataSeries[i].next );
		const size_t nextIdx = ( i + 1u < numDataSeries ) ? ( i + 1u ) : 0u;
		Ogre::Vector2 next =
			spoke * ( bDrawCurr ? m_dataSeries[nextIdx].curr : m_dataSeries[nextIdx].next );

		current = rotate( current, rot * static_cast<Ogre::Real>( i ) );
		next = rotate( next, rot * static_cast<Ogre::Real>( nextIdx ) );

		// Displace origin current and next by lineThickness.
		Ogre::Vector2 origin = ( ( current + next ) * 0.5f ).normalisedCopy() * lineThickness * 0.5f;
		Ogre::Vector2 finalCurr =  //
			current +              //
			Ogre::Math::lerp( -current.normalisedCopy(), ( next - current ).normalisedCopy(),
							  1.0f / 3.0f )
					.normalisedCopy() *
				lineThickness;
		Ogre::Vector2 finalNext =
			next +
			Ogre::Math::lerp( -next.normalisedCopy(), ( current - next ).normalisedCopy(), 1.0f / 3.0f )
					.normalisedCopy() *
				lineThickness;

		setTriangle( triOffset * 3u,  //
					 origin * proportion, finalCurr * proportion, finalNext * proportion, shapeColor );
		triOffset++;
	}
#endif

	return triOffset;
}
//-------------------------------------------------------------------------
size_t RadarChart::drawBackgroundShape( size_t triOffset, const Ogre::ColourValue darkColor,
										const Ogre::ColourValue backgroundColor )
{
	const size_t numDataSeries = m_dataSeries.size();

	const uint32_t numRings = m_displaySettings.numRings;
	const float topRingThickness = m_displaySettings.bgTopRingThickness;
	const float ringThickness = m_displaySettings.bgRingThickness;
	const float spokeThickness = m_displaySettings.bgSpokeThickness;

	const Ogre::Degree rot( Ogre::Real( 360.0f ) / Ogre::Real( numDataSeries ) );

	const Ogre::Vector2 spoke( 0, -m_displaySettings.chartSize );
	const Ogre::Vector2 proportion = m_proportion;

	for( size_t i = 0; i < numDataSeries; ++i )
	{
		const size_t nextIdx = ( i + 1u < numDataSeries ) ? ( i + 1u ) : 0u;
		setTriangle( triOffset * 3u, Ogre::Vector2( 0, 0 ),
					 rotate( spoke, rot * static_cast<Ogre::Real>( i ) ) * proportion,
					 rotate( spoke, rot * static_cast<Ogre::Real>( nextIdx ) ) * proportion, darkColor );
		triOffset++;
	}

	const float totalReservedThickness =
		static_cast<float>( numRings - 1u ) * ringThickness + topRingThickness;
	const float totalAvailableThickness = 1.0f - totalReservedThickness;
	const float pentagonThickness = totalAvailableThickness / static_cast<float>( numRings );

	for( size_t k = 0; k < numRings; ++k )
	{
		const float pentagonTop =
			1.0f - ( pentagonThickness + ringThickness ) * (float)k - topRingThickness * 1.5f;
		const float pentagonBottom =
			( k + 1u == numRings ) ? ringThickness : ( pentagonTop - pentagonThickness );

		const Ogre::Radian rotOffsetTop( 0.5f * spokeThickness / pentagonTop );
		const Ogre::Radian rotOffsetBottom( 0.5f * spokeThickness / pentagonBottom );

		for( size_t i = 0; i < numDataSeries; ++i )
		{
			const size_t nextIdx = ( i + 1u < numDataSeries ) ? ( i + 1u ) : 0u;

			Ogre::Vector2 topRing = spoke;
			topRing.y *= pentagonTop;

			Ogre::Vector2 botRing = spoke;
			botRing.y *= pentagonBottom;

			setTriangle(
				triOffset * 3u,  //
				rotate( topRing, rot * static_cast<Ogre::Real>( i ) + rotOffsetTop ) * proportion,
				rotate( botRing, rot * static_cast<Ogre::Real>( i ) + rotOffsetBottom ) * proportion,
				rotate( topRing, rot * static_cast<Ogre::Real>( nextIdx ) - rotOffsetTop ) * proportion,
				backgroundColor );
			triOffset++;

			setTriangle(
				triOffset * 3u,  //
				rotate( topRing, rot * static_cast<Ogre::Real>( nextIdx ) - rotOffsetTop ) * proportion,
				rotate( botRing, rot * static_cast<Ogre::Real>( i ) + rotOffsetBottom ) * proportion,
				rotate( botRing, rot * static_cast<Ogre::Real>( nextIdx ) - rotOffsetBottom ) *
					proportion,
				backgroundColor );
			triOffset++;
		}
	}

	return triOffset;
}
//-------------------------------------------------------------------------
void RadarChart::drawRadarChart()
{
#ifdef USE_PENTAGONS
	const size_t mainShapeNumTris = 3u;
#else
	const size_t mainShapeNumTris = 2u;
#endif
	// 2u for the background quad
	//	1x for shape background
	//	2x for each ring.
	//	2x (or 3x if using pentagons) for the actual drawing
	const size_t numTriangles =
		2u + m_dataSeries.size() * ( 1u + m_displaySettings.numRings * 2u + mainShapeNumTris );
	setNumTriangles( numTriangles );

	size_t triOffset = 0u;

	// Draw the background
	setTriangle( triOffset * 3u, Ogre::Vector2( -1, -1 ), Ogre::Vector2( 1, 1 ), Ogre::Vector2( 1, -1 ),
				 m_displaySettings.emptyBackgroundColor );
	triOffset++;
	setTriangle( triOffset * 3u, Ogre::Vector2( 1, 1 ), Ogre::Vector2( -1, -1 ), Ogre::Vector2( -1, 1 ),
				 m_displaySettings.emptyBackgroundColor );
	triOffset++;

	triOffset = drawBackgroundShape( triOffset, m_displaySettings.darkColor,
									 m_displaySettings.radarBackgroundColor );

	// Draw the chart
	triOffset = drawChartShape( triOffset, m_displaySettings.lightColor, m_displaySettings.fgLineColor,
								m_displaySettings.fgLineThickness, true );

	COLIBRI_ASSERT_LOW( triOffset == getNumTriangles() );
}
//-------------------------------------------------------------------------
void RadarChart::setDisplaySettings( const DisplaySettings &displaySettings )
{
	m_displaySettings = displaySettings;
}
//-------------------------------------------------------------------------
void RadarChart::setPropotion( const Ogre::Vector2 &proportion )
{
	m_proportion = proportion;
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
	drawRadarChart();
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

	const Ogre::Degree rot( Ogre::Real( 360.0f ) / Ogre::Real( numDataSeries ) );

	const Ogre::Vector2 spoke(
		0, -( m_displaySettings.chartSize + m_displaySettings.chartToLabelDistance ) );
	const Ogre::Vector2 proportion = m_proportion;

	for( size_t i = 0; i < numDataSeries; ++i )
	{
		const Ogre::Vector2 ndcPos = rotate( spoke, rot * static_cast<Ogre::Real>( i ) ) * proportion;
		Ogre::Vector2 canvasPos = ( ndcPos * 0.5f + 0.5f ) * m_size;

		canvasPos = canvasPos - m_labels[i]->getSize() * 0.5f;
		canvasPos.makeCeil( Ogre::Vector2::ZERO );
		canvasPos.makeFloor( m_size - m_labels[i]->getSize() );

		m_labels[i]->setTopLeft( canvasPos );
	}
}
