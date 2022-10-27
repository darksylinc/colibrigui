
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
									  std::to_string( m_dataSeries[i].value * fScale ) );
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
