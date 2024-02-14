#include "ColibriGui/ColibriOffScreenCanvas.h"

#include "ColibriGui/ColibriManager.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiDef.h"
#include "ColibriGui/Ogre/CompositorPassColibriGuiProvider.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreDepthBuffer.h"
#include "OgreHlmsManager.h"
#include "OgreId.h"
#include "OgreLwString.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreTextureGpuManager.h"

using namespace Colibri;

static const char *kOffscreenDefaultWorkspaceName = "!#ColibriOffScreenCanvasWorkspace";

OffScreenCanvas::OffScreenCanvas( ColibriManager *primaryManager ) :
	m_secondaryManager( new ColibriManager( primaryManager->getLogListener(),
											primaryManager->getColibriListener(), true ) ),
	m_workspace( 0 ),
	m_canvasTexture( 0 )
{
	m_secondaryManager->_setPrimary( primaryManager );
}
//-----------------------------------------------------------------------------
OffScreenCanvas::~OffScreenCanvas()
{
	destroyWorkspace();
	if( m_canvasTexture )
	{
		m_canvasTexture->getTextureManager()->destroyTexture( m_canvasTexture );
		m_canvasTexture = 0;
	}
	delete m_secondaryManager;
}
//-----------------------------------------------------------------------------
void OffScreenCanvas::createWorkspaceDefinition()
{
	using namespace Ogre;
	CompositorManager2 *compositorManager = m_secondaryManager->getOgreRoot()->getCompositorManager2();

	const static String kOffscreenDefaultNodeName = "!#ColibriOffScreenCanvasNode";

	if( compositorManager->hasNodeDefinition( kOffscreenDefaultNodeName ) )
		return;

	CompositorNodeDef *nodeDef = compositorManager->addNodeDefinition( kOffscreenDefaultNodeName );
	nodeDef->addTextureSourceName( "rt_output", 0, TextureDefinitionBase::TEXTURE_INPUT );

	nodeDef->setNumTargetPass( 1u );
	CompositorTargetDef *targetDef = nodeDef->addTargetPass( "rt_output" );
	targetDef->setNumPasses( 1u );

	CompositorPassColibriGuiDef *passColibri =
		static_cast<CompositorPassColibriGuiDef *>( targetDef->addPass( PASS_CUSTOM, "colibri_gui" ) );
	passColibri->mSkipLoadStoreSemantics = false;  // We must clear the RT, we are the only pass.
	passColibri->setAllLoadActions( LoadAction::Clear );
	passColibri->mStoreActionDepth = StoreAction::DontCare;
	passColibri->mStoreActionStencil = StoreAction::DontCare;

	CompositorWorkspaceDef *workDef =
		compositorManager->addWorkspaceDefinition( kOffscreenDefaultWorkspaceName );
	workDef->connectExternal( 0, kOffscreenDefaultNodeName, 0 );
}
//-----------------------------------------------------------------------------
void OffScreenCanvas::createWorkspace(
	Ogre::CompositorPassColibriGuiProvider *colibriCompositorProvider )
{
	using namespace Ogre;
	CompositorManager2 *compositorManager = m_secondaryManager->getOgreRoot()->getCompositorManager2();

	destroyWorkspace();
	createWorkspaceDefinition();

	ColibriManager *oldValue = colibriCompositorProvider->getColibriManager();
	colibriCompositorProvider->_setColibriManager( m_secondaryManager );

	m_workspace =
		compositorManager->addWorkspace( m_secondaryManager->getOgreSceneManager(), m_canvasTexture, 0,
										 kOffscreenDefaultWorkspaceName, false );

	colibriCompositorProvider->_setColibriManager( oldValue );
}
//-----------------------------------------------------------------------------
void OffScreenCanvas::destroyWorkspace()
{
	if( m_workspace )
	{
		m_workspace->getCompositorManager()->removeWorkspace( m_workspace );
		m_workspace = 0;
	}
}
//-----------------------------------------------------------------------------
void OffScreenCanvas::createTexture( uint32_t width, uint32_t height, Ogre::PixelFormatGpu pixelFormat )
{
	if( m_canvasTexture && m_canvasTexture->getWidth() == width && m_canvasTexture->getHeight() &&
		m_canvasTexture->getPixelFormat() == pixelFormat )
	{
		return;
	}

	using namespace Ogre;

	TextureGpuManager *textureManager =
		m_secondaryManager->getOgreHlmsManager()->getRenderSystem()->getTextureGpuManager();

	if( m_canvasTexture )
	{
		textureManager->destroyTexture( m_canvasTexture );
		m_canvasTexture = 0;
	}

	{
		char tmpBuffer[64];
		LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
		texName.a( "ColibriOffscreenCanvas", Id::generateNewId<OffScreenCanvas>() );
		m_canvasTexture =
			textureManager->createTexture( texName.c_str(), GpuPageOutStrategy::Discard,
										   TextureFlags::RenderToTexture, TextureTypes::Type2D );
	}

	m_canvasTexture->setResolution( width, height );
	m_canvasTexture->setPixelFormat( pixelFormat );
	m_canvasTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_NULL );
	m_canvasTexture->scheduleTransitionTo( GpuResidency::Resident );
}
//-----------------------------------------------------------------------------
void OffScreenCanvas::updateCanvas( const float timeSinceLast )
{
	m_secondaryManager->update( timeSinceLast );

	// m_workspace->_beginUpdate( true );
	m_workspace->_update();
	// m_workspace->_endUpdate( true );
}
