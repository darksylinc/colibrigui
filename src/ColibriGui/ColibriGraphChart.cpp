
#include "ColibriGui/ColibriGraphChart.h"

#include "ColibriGui/ColibriManager.h"

#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreRenderSystem.h"
#include "OgreStagingTexture.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"

using namespace Colibri;

GraphChart::GraphChart( ColibriManager *manager ) : CustomShape( manager ), m_textureData( 0 )
{
	setCustomParameter( 6374, Ogre::Vector4( 1.0f ) );
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

	setNumTriangles( 2u );
	setQuad( 0u, Ogre::Vector2( -1, -1 ), Ogre::Vector2( 2.0f ), Ogre::ColourValue::White,
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

	// No need to destroy m_labels. They are our children and will be destroyed
	// automatically with us.
	CustomShape::_destroy();
}
//-------------------------------------------------------------------------
void GraphChart::setMaxValues( uint32_t numColumns, uint32_t maxEntriesPerColumn )
{
	m_columns.resize( numColumns );
	m_allValues.clear();
	m_allValues.resize( numColumns * maxEntriesPerColumn, 0u );

	for( size_t y = 0u; y < numColumns; ++y )
	{
		m_columns[y].values = &m_allValues[y * maxEntriesPerColumn];
	}

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

	// Set the texture to our datablock.
	Ogre::HlmsManager *hlmsManager = m_manager->getOgreHlmsManager();
	for( size_t i = 0; i < States::NumStates; ++i )
	{
		Ogre::HlmsDatablock *datablock =
			hlmsManager->getDatablockNoDefault( m_stateInformation[i].materialName );

		COLIBRI_ASSERT_HIGH( dynamic_cast<Ogre::HlmsUnlitDatablock *>( datablock ) );
		Ogre::HlmsUnlitDatablock *unlitDatablock = static_cast<Ogre::HlmsUnlitDatablock *>( datablock );
		unlitDatablock->setTexture( 0u, m_textureData );
	}

	syncChart();
}
//-------------------------------------------------------------------------
uint32_t GraphChart::getEntriesPerColumn() const
{
	if( !m_textureData )
		return 0u;
	return m_textureData->getWidth();
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

	for( size_t y = 0u; y < textureBox.height; ++y )
	{
		memcpy( textureBox.at( 0u, y, 0u ), m_columns[y].values, textureBox.width * sizeof( uint16_t ) );
	}

	stagingTexture->stopMapRegion();
	stagingTexture->upload( textureBox, m_textureData, 0u );

	// Workaround OgreNext bug where calling syncChart() multiple times in a row causes Vulkan
	// race conditions because it doesn't place barriers to protect each copy as it assumes
	// multiple write copies to the same textures are to be done to non-overlapping regions.
	m_manager->getOgreHlmsManager()->getRenderSystem()->endCopyEncoder();
}
