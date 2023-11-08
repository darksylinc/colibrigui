
#include "ColibriGui/ColibriGraphChart.h"

#include "ColibriGui/ColibriLabel.h"
#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/Layouts/ColibriLayoutLine.h"
#include "ColibriGui/Layouts/ColibriLayoutTableSameSize.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreRenderSystem.h"
#include "OgreStagingTexture.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"

using namespace Colibri;

GraphChart::Params::Params() :
	numLines( 5u ),
	lineThickness( 0.01f ),
	graphInnerTopLeft( Ogre::Vector2( 0.1f ) ),
	graphInnerSize( Ogre::Vector2( 0.8f ) ),
	lineColour( Ogre::ColourValue::White ),
	bgInnerColour( Ogre::ColourValue( 0.0f, 0.0f, 0.0f, 0.5f ) ),
	bgOuterColour( Ogre::ColourValue( 0.0f, 0.0f, 0.0f, 0.75f ) )
{
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
GraphChart::GraphChart( ColibriManager *manager ) :
	CustomShape( manager ),
	m_textureData( 0 ),
	m_labelsDirty( true ),
	m_autoMin( false ),
	m_autoMax( false ),
	m_markersOnRight( false ),
	m_autoMinMaxRounding( 0.0f ),
	m_minSample( 0.0f ),
	m_maxSample( 1.0f ),
	m_lastMinValue( std::numeric_limits<float>::max() ),
	m_lastMaxValue( -std::numeric_limits<float>::max() ),
	m_labelPrecision( 2 )
{
	setCustomParameter( 6375, Ogre::Vector4( 1.0f ) );
}
//-------------------------------------------------------------------------
void GraphChart::_initialize()
{
	CustomShape::_initialize();
	_setSkinPack( m_manager->getDefaultSkin( SkinWidgetTypes::GraphChart ) );

	// We must clone our datablocks because they will have a unique texture.
	Ogre::HlmsManager *hlmsManager = m_manager->getOgreHlmsManager();
	for( size_t i = 0; i < States::NumStates; ++i )
	{
		Ogre::HlmsDatablock *refDatablock =
			hlmsManager->getDatablockNoDefault( m_stateInformation[i].materialName );

		char tmpBuffer[64];
		Ogre::LwString idStr( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
		idStr.a( "_", getId() );
		const Ogre::String newName = *refDatablock->getNameStr() + idStr.c_str();

		Ogre::HlmsDatablock *datablock = hlmsManager->getDatablockNoDefault( newName );
		if( !datablock )
			datablock = refDatablock->clone( newName );

		m_stateInformation[i].materialName = datablock->getName();

		if( i == m_currentState )
			setDatablock( datablock );
	}

	{
		const Ogre::String defaultDatablockName( "ColibriDefaultBlankDatablock" );
		Ogre::HlmsDatablock *refDatablock = hlmsManager->getDatablockNoDefault( defaultDatablockName );
		if( !refDatablock )
		{
			Ogre::HlmsDatablock *defaultDatablock =
				hlmsManager->getHlms( Ogre::HLMS_UNLIT )->getDefaultDatablock();
			refDatablock = defaultDatablock->clone( defaultDatablockName );
			Ogre::HlmsMacroblock macroblock;
			macroblock.mDepthCheck = false;
			macroblock.mDepthWrite = false;
			refDatablock->setMacroblock( macroblock );
		}
	}

	setNumTriangles( 2u );
	setQuad( 0u, Ogre::Vector2( -1.0f ), Ogre::Vector2( 2.0f ), Ogre::ColourValue::White,
			 Ogre::Vector2( 0.0f, 1.0f ), Ogre::Vector2( 1.0f, -1.0f ) );
}
//-------------------------------------------------------------------------
void GraphChart::_destroy()
{
	this->_setNullDatablock();

	// Destroy the datablock clones we created.
	Ogre::HlmsManager *hlmsManager = m_manager->getOgreHlmsManager();
	std::set<Ogre::IdString> seenDatablocks;
	for( size_t i = 0; i < States::NumStates; ++i )
	{
		if( seenDatablocks.count( m_stateInformation[i].materialName ) != 0u )
		{
			Ogre::HlmsDatablock *datablock =
				hlmsManager->getDatablockNoDefault( m_stateInformation[i].materialName );
			datablock->getCreator()->destroyDatablock( m_stateInformation[i].materialName );
			seenDatablocks.insert( m_stateInformation[i].materialName );
		}
	}

	if( m_textureData )
	{
		Ogre::TextureGpuManager *textureManager = m_textureData->getTextureManager();
		textureManager->destroyTexture( m_textureData );
		m_textureData = 0;
	}

	// No need to destroy m_markers. They are our children and will be destroyed
	// automatically with us.
	CustomShape::_destroy();
}
//-------------------------------------------------------------------------
void GraphChart::setMaxValues( const uint32_t numColumns, const uint32_t maxEntriesPerColumn )
{
	const size_t oldNumColumns = m_columns.size();

	if( numColumns < oldNumColumns )
	{
		// Destroy excess widgets.
		std::vector<Column>::iterator itor = m_columns.begin() + numColumns;
		std::vector<Column>::iterator endt = m_columns.end();

		while( itor != endt )
		{
			m_manager->destroyWidget( itor->label );
			m_manager->destroyWidget( itor->rectangle );
			++itor;
		}
	}

	m_columns.resize( numColumns );
	m_allValues.clear();
	m_allValues.resize( numColumns * maxEntriesPerColumn, 0.0f );

	for( size_t y = 0u; y < numColumns; ++y )
	{
		m_columns[y].values = &m_allValues[y * maxEntriesPerColumn];

		if( y >= oldNumColumns )
		{
			m_columns[y].label = m_manager->createWidget<Label>( this );
			m_columns[y].rectangle = m_manager->createWidget<CustomShape>( this );

			m_columns[y].rectangle->setDatablock( "ColibriDefaultBlankDatablock" );
			m_columns[y].rectangle->setNumTriangles( 2u );
			m_columns[y].rectangle->setQuad( 0u, Ogre::Vector2( -1.0f ), Ogre::Vector2( 2.0f ),
											 Ogre::ColourValue::White, Ogre::Vector2::ZERO,
											 Ogre::Vector2::UNIT_SCALE );

			m_columns[y].label->m_gridLocation = GridLocations::CenterLeft;
			m_columns[y].rectangle->m_gridLocation = GridLocations::CenterLeft;
			m_columns[y].label->m_margin = 10.0f;
		}
	}

	m_labelsDirty = true;

	if( m_textureData && m_textureData->getHeight() == numColumns &&
		m_textureData->getWidth() == maxEntriesPerColumn )
	{
		// We're done.
		return;
	}

	if( m_textureData )
	{
		m_textureData->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
	}
	else
	{
		char tmpBuffer[64];
		Ogre::LwString texName( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
		texName.a( "GraphChart #", getId() );

		Ogre::TextureGpuManager *textureManager =
			m_manager->getOgreHlmsManager()->getRenderSystem()->getTextureGpuManager();

		m_textureData = textureManager->createTexture(
			texName.c_str(), Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::ManualTexture,
			Ogre::TextureTypes::Type2D );
	}

	m_textureData->setResolution( maxEntriesPerColumn, numColumns );
	m_textureData->setPixelFormat( Ogre::PFG_R16_UNORM );
	m_textureData->scheduleTransitionTo( Ogre::GpuResidency::Resident );

	Ogre::HlmsManager *hlmsManager = m_manager->getOgreHlmsManager();

	// Set the texture to our datablock.
	for( size_t i = 0; i < States::NumStates; ++i )
	{
		Ogre::HlmsDatablock *datablock =
			hlmsManager->getDatablockNoDefault( m_stateInformation[i].materialName );

		COLIBRI_ASSERT_HIGH( dynamic_cast<Ogre::HlmsUnlitDatablock *>( datablock ) );
		Ogre::HlmsUnlitDatablock *unlitDatablock = static_cast<Ogre::HlmsUnlitDatablock *>( datablock );
		unlitDatablock->setTexture( 0u, m_textureData );
	}
}
//-------------------------------------------------------------------------
uint32_t GraphChart::getEntriesPerColumn() const
{
	if( !m_textureData )
		return 0u;
	return m_textureData->getWidth();
}
//-------------------------------------------------------------------------
void GraphChart::setDataRange( bool autoMin, bool autoMax, float minValue, float maxValue,
							   float autoMinMaxRounding )
{
	m_autoMin = autoMin;
	m_autoMax = autoMax;
	m_autoMinMaxRounding = autoMinMaxRounding;
	m_minSample = minValue;
	m_maxSample = maxValue;
	m_labelsDirty = true;
}
//-------------------------------------------------------------------------
void GraphChart::positionGraphLegend()
{
	float labelHeight = std::numeric_limits<float>::max();
	for( const Column &column : m_columns )
		labelHeight = std::min( labelHeight, column.label->getSize().y );
	for( const Column &column : m_columns )
	{
		column.rectangle->setSizeAndCellMinSize( Ogre::Vector2( labelHeight * 1.33f, labelHeight ) *
												 0.33f );
	}

	const size_t numColumns = m_columns.size();

	LayoutTableSameSize rootLayout( m_manager );

	rootLayout.m_numColumns = numColumns;

	std::vector<LayoutLine> columnLines;
	columnLines.resize( m_columns.size(), m_manager );

	{
		const Ogre::Vector2 topLeftCornerSize = m_params.graphInnerTopLeft * getSize();
		if( topLeftCornerSize.x > topLeftCornerSize.y && topLeftCornerSize.y < labelHeight )
			rootLayout.m_numColumns = 1u;
	}

	for( size_t y = 0u; y < numColumns; ++y )
	{
		columnLines[y].m_vertical = false;
		columnLines[y].m_expand[0] = true;
		columnLines[y].m_expand[1] = true;
		columnLines[y].m_proportion[0] = 1u;
		columnLines[y].m_proportion[1] = 1u;
		columnLines[y].addCell( m_columns[y].rectangle );
		columnLines[y].addCell( m_columns[y].label );
		rootLayout.addCell( &columnLines[y] );
	}

	rootLayout.layout();
}
//-------------------------------------------------------------------------
void GraphChart::positionMarkersInLines( const float minValue, const float maxValue )
{
	const size_t numLines = m_params.numLines;

	const Ogre::Vector2 graphSize = getSize();

	// In virtual canvas units.
	const float lineThickness = m_params.lineThickness;
	const Ogre::Vector2 graphInnerSize = m_params.graphInnerSize * graphSize;
	const Ogre::Vector2 graphInnerTopLeft =
		graphSize * ( m_params.graphInnerTopLeft - Ogre::Vector2( 0.0f, lineThickness ) );

	for( size_t i = 0u; i < numLines; ++i )
	{
		const float fW = float( i ) / float( numLines - 1u );
		const float value = Ogre::Math::lerp( maxValue, minValue, fW );
		char tmpBuffer[64];
		Ogre::LwString valStr( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
		m_markers[i]->setText( valStr.a( Ogre::LwString::Float( value, m_labelPrecision ) ).c_str() );
		m_markers[i]->sizeToFit();

		float leftPos;

		const float margin = 5.0f;
		if( m_markersOnRight )
			leftPos = graphInnerSize.x + margin;
		else
			leftPos = -m_markers[i]->getSize().x - margin;

		// Honestly I can't make sense of this math ('top' and 'intervalLength' calculation).
		// What I thought would work didn't. I arrived to this formula by trial and error. It just works.
		const float intervalLength =
			graphInnerSize.y * ( ( 1.0f - lineThickness ) / float( numLines - 1u ) );
		const float top = float( i ) * intervalLength + lineThickness * 1.5f * graphInnerSize.y;
		m_markers[i]->setTopLeft( graphInnerTopLeft +
								  Ogre::Vector2( leftPos, top - m_markers[i]->getSize().y * 0.5f ) );
	}
}
//-------------------------------------------------------------------------
void GraphChart::positionLabels()
{
	if( !m_labelsDirty )
		return;

	positionGraphLegend();
	positionMarkersInLines( m_lastMinValue, m_lastMaxValue );

	m_labelsDirty = false;
}
//-------------------------------------------------------------------------
void GraphChart::syncChart()
{
	Ogre::TextureGpuManager *textureManager = m_textureData->getTextureManager();
	Ogre::StagingTexture *stagingTexture = textureManager->getStagingTexture(
		m_textureData->getWidth(), m_textureData->getHeight(), 1u, 1u, Ogre::PFG_R16_UNORM, 100u );

	stagingTexture->startMapRegion();
	Ogre::TextureBox textureBox = stagingTexture->mapRegion(
		m_textureData->getWidth(), m_textureData->getHeight(), 1u, 1u, Ogre::PFG_R16_UNORM );

	COLIBRI_ASSERT_MEDIUM( m_textureData->getWidth() == getEntriesPerColumn() );
	COLIBRI_ASSERT_MEDIUM( m_textureData->getHeight() == m_columns.size() );

	float minSample = m_minSample;
	float maxSample = m_maxSample;

	{
		const bool autoMin = m_autoMin;
		const bool autoMax = m_autoMax;

		if( autoMin || autoMax )
		{
			for( size_t y = 0u; y < textureBox.height; ++y )
			{
				for( size_t x = 0u; x < textureBox.width; ++x )
				{
					if( autoMin )
						minSample = std::min( m_columns[y].values[x], minSample );
					if( autoMax )
						maxSample = std::max( m_columns[y].values[x], maxSample );
				}
			}

			if( m_autoMinMaxRounding != 0.0f )
			{
				if( autoMin )
					minSample = std::round( minSample / m_autoMinMaxRounding ) * m_autoMinMaxRounding;
				if( autoMax )
					maxSample = std::round( maxSample / m_autoMinMaxRounding ) * m_autoMinMaxRounding;
			}
		}
	}

	const float sampleInterval =
		( maxSample - minSample ) < 1e-6f ? 1.0f : ( 1.0f / ( maxSample - minSample ) );

	for( size_t y = 0u; y < textureBox.height; ++y )
	{
		uint16_t *RESTRICT_ALIAS dstData =
			reinterpret_cast<uint16_t * RESTRICT_ALIAS>( textureBox.at( 0u, y, 0u ) );
		for( size_t x = 0u; x < textureBox.width; ++x )
		{
			float fValue =
				Ogre::Math::saturate( ( m_columns[y].values[x] - minSample ) * sampleInterval );
			dstData[x] = static_cast<uint16_t>( fValue * 65535.0f );
		}
	}

	stagingTexture->stopMapRegion();
	stagingTexture->upload( textureBox, m_textureData, 0u );
	textureManager->removeStagingTexture( stagingTexture );

	// Workaround OgreNext bug where calling syncChart() multiple times in a row causes Vulkan
	// race conditions because it doesn't place barriers to protect each copy as it assumes
	// multiple write copies to the same textures are to be done to non-overlapping regions.
	m_manager->getOgreHlmsManager()->getRenderSystem()->endCopyEncoder();

	if( m_lastMinValue != minSample || m_lastMaxValue != maxSample )
	{
		m_lastMinValue = minSample;
		m_lastMaxValue = maxSample;
		m_labelsDirty = true;
	}

	positionLabels();
}
//-------------------------------------------------------------------------
void GraphChart::setMarkersFontSize( FontSize fontSize )
{
	for( Colibri::Label *label : m_markers )
		label->setDefaultFontSize( fontSize );
	m_labelsDirty = true;
}
//-------------------------------------------------------------------------
void GraphChart::build( const Params &params )
{
	m_params = params;

	const size_t oldNumLabels = m_markers.size();
	const size_t newNumLabels = m_params.numLines + 1u;
	if( newNumLabels < oldNumLabels )
	{
		// Destroy excess labels.
		std::vector<Colibri::Label *>::iterator itor = m_markers.begin() + newNumLabels;
		std::vector<Colibri::Label *>::iterator endt = m_markers.end();

		while( itor != endt )
			m_manager->destroyWidget( *itor++ );
	}
	m_markers.resize( newNumLabels );
	for( size_t i = oldNumLabels; i < newNumLabels; ++i )
		m_markers[i] = m_manager->createWidget<Colibri::Label>( this );

	Ogre::HlmsDatablock *datablock = mHlmsDatablock;
	_setNullDatablock();
	setDatablock( datablock );

	m_labelsDirty = true;
	syncChart();
}
//-------------------------------------------------------------------------
void GraphChart::setTransformDirty( uint32_t dirtyReason )
{
	// Only update if our size is directly being changed, not our parent's
	if( ( dirtyReason & ( TransformDirtyParentCaller | TransformDirtyScale ) ) == TransformDirtyScale )
	{
		m_labelsDirty = true;
		positionLabels();
	}

	Widget::setTransformDirty( dirtyReason );
}
