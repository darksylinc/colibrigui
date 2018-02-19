
#include "CrystalGui/CrystalManager.h"

#include "CrystalGui/CrystalWindow.h"
#include "CrystalGui/CrystalSkinManager.h"

#include "CrystalGui/Ogre/CrystalOgreRenderable.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Math/Array/OgreObjectMemoryManager.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreRoot.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbDrawCall.h"

namespace Crystal
{
	static LogListener DefaultLogListener;
	static const Ogre::HlmsCache c_dummyCache( 0, Ogre::HLMS_MAX, Ogre::HlmsPso() );

	CrystalManager::CrystalManager() :
		m_numWidgets( 0 ),
		m_logListener( &DefaultLogListener ),
		m_windowNavigationDirty( false ),
		m_childrenNavigationDirty( false ),
		m_root( 0 ),
		m_vaoManager( 0 ),
		m_objectMemoryManager( 0 ),
		m_sceneManager( 0 ),
		m_vao( 0 ),
		m_indirectBuffer( 0 ),
		m_commandBuffer( 0 ),
		m_skinManager( 0 )
	{
		setCanvasSize( Ogre::Vector2( 1.0f ), Ogre::Vector2( 1.0f / 1600.0f, 1.0f / 900.0f ) );

		m_skinManager = new SkinManager( this );
	}
	//-------------------------------------------------------------------------
	CrystalManager::~CrystalManager()
	{
		setOgre( 0, 0, 0 );
		delete m_skinManager;
		m_skinManager = 0;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::loadSkins( const char *fullPath )
	{
		m_skinManager->loadSkins( fullPath );
	}
	//-------------------------------------------------------------------------
	void CrystalManager::setOgre( Ogre::Root * crystalgui_nullable root,
								  Ogre::VaoManager * crystalgui_nullable vaoManager,
								  Ogre::SceneManager * crystalgui_nullable sceneManager )
	{
		delete m_commandBuffer;
		m_commandBuffer = 0;
		if( m_indirectBuffer )
		{
			m_vaoManager->destroyIndirectBuffer( m_indirectBuffer );
			m_indirectBuffer = 0;
		}
		if( m_vao )
		{
			Ogre::CrystalOgreRenderable::destroyVao( m_vao, m_vaoManager );
			m_vao = 0;
		}
		/*if( m_defaultIndexBuffer )
		{
			m_vaoManager->destroyIndexBuffer( m_defaultIndexBuffer );
			m_defaultIndexBuffer = 0;
		}*/
		delete m_objectMemoryManager;
		m_objectMemoryManager = 0;

		m_root = root;
		m_vaoManager = vaoManager;
		m_sceneManager = sceneManager;

		if( vaoManager )
		{
			m_objectMemoryManager = new Ogre::ObjectMemoryManager();
			//m_defaultIndexBuffer = Ogre::CrystalOgreRenderable::createIndexBuffer( vaoManager );
			m_vao = Ogre::CrystalOgreRenderable::createVao( 6u * 9u, vaoManager );
			size_t requiredBytes = 1u * sizeof( Ogre::CbDrawStrip );
			m_indirectBuffer = m_vaoManager->createIndirectBuffer( requiredBytes,
																   Ogre::BT_DYNAMIC_PERSISTENT,
																   0, false );
			m_commandBuffer = new Ogre::CommandBuffer();
			m_commandBuffer->setCurrentRenderSystem( m_sceneManager->getDestinationRenderSystem() );
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::setCanvasSize( const Ogre::Vector2 &canvasSize, const Ogre::Vector2 &pixelSize )
	{
		m_canvasSize = canvasSize;
		m_invCanvasSize2x = 2.0f / canvasSize;
		m_pixelSize = pixelSize / canvasSize;
	}
	//-------------------------------------------------------------------------
	Window* CrystalManager::createWindow( Window * crystalgui_nullable parent )
	{
		Window *retVal = new Window( this );

		if( !parent )
			m_windows.push_back( retVal );
		else
		{
			retVal->m_parent = parent;
			parent->m_childWindows.push_back( retVal );
		}

		retVal->setWindowNavigationDirty();

		++m_numWidgets;

		return retVal;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::destroyWindow( Window *window )
	{
		if( window->m_parent )
		{
			WindowVec::iterator itor = std::find( m_windows.begin(), m_windows.end(), window );

			if( itor == m_windows.end() )
			{
				m_logListener->log( "Window does not belong to this CrystalManager! "
									"Double free perhaps?", LogSeverity::Fatal );
			}
			else
				m_windows.erase( itor );
		}

		window->_destroy();
		delete window;

		--m_numWidgets;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::destroyWidget( Widget *widget )
	{
		if( widget->isWindow() )
		{
			assert( dynamic_cast<Window*>( widget ) );
			destroyWindow( static_cast<Window*>( widget ) );
		}
		else
		{
			widget->_destroy();
			delete widget;
			--m_numWidgets;
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::_setAsParentlessWindow( Window *window )
	{
		m_windows.push_back( window );
	}
	//-------------------------------------------------------------------------
	void CrystalManager::setAsParentlessWindow( Window *window )
	{
		if( window->m_parent )
		{
			window->detachFromParent();
			m_windows.push_back( window );
		}
	}
	//-----------------------------------------------------------------------------------
	void CrystalManager::checkVertexBufferCapacity()
	{
		const Ogre::uint32 requiredVertexCount = static_cast<Ogre::uint32>( m_numWidgets * (6u * 9u) );

		Ogre::VertexBufferPacked *vertexBuffer = m_vao->getBaseVertexBuffer();
		const Ogre::uint32 currVertexCount = vertexBuffer->getNumElements();
		if( requiredVertexCount > vertexBuffer->getNumElements() )
		{
			const Ogre::uint32 newVertexCount = std::max( requiredVertexCount,
														  currVertexCount + (currVertexCount >> 1u) );
			Ogre::CrystalOgreRenderable::destroyVao( m_vao, m_vaoManager );
			m_vao = Ogre::CrystalOgreRenderable::createVao( newVertexCount, m_vaoManager );

			WindowVec::const_iterator itor = m_windows.begin();
			WindowVec::const_iterator end  = m_windows.end();

			while( itor != end )
			{
				(*itor)->broadcastNewVao( m_vao );
				++itor;
			}

			if( m_indirectBuffer->getMappingState() != Ogre::MS_UNMAPPED )
				m_indirectBuffer->unmap( Ogre::UO_UNMAP_ALL );
			m_vaoManager->destroyIndirectBuffer( m_indirectBuffer );
			size_t requiredBytes = m_numWidgets * sizeof( Ogre::CbDrawStrip );
			m_indirectBuffer = m_vaoManager->createIndirectBuffer( requiredBytes,
																   Ogre::BT_DYNAMIC_PERSISTENT,
																   0, false );
		}
	}
	//-------------------------------------------------------------------------
	template <typename T>
	void CrystalManager::autosetNavigation( const std::vector<T> &container )
	{
		typename std::vector<T>::const_iterator itor = container.begin();
		typename std::vector<T>::const_iterator end  = container.end();

		while( itor != end )
		{
			Widget *widget = *itor;

			Widget *closestSiblings[Borders::NumBorders] = { 0, 0, 0, 0 };
			float closestSiblingDistances[Borders::NumBorders] =
			{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			};

			typename std::vector<T>::const_iterator it2 = itor + 1u;
			while( it2 != end )
			{
				Widget *widget2 = *it2;

				const Ogre::Vector2 cornerToCorner[4] =
				{
					widget2->m_position -
					widget->m_position,

					Ogre::Vector2( widget2->getRight(), widget2->m_position.y ) -
					Ogre::Vector2( widget->getRight(), widget->m_position.y ),

					Ogre::Vector2( widget2->m_position.x, widget2->getBottom() ) -
					Ogre::Vector2( widget->m_position.x, widget->getBottom() ),

					Ogre::Vector2( widget2->getRight(), widget2->getBottom() ) -
					Ogre::Vector2( widget->getRight(), widget->getBottom() ),
				};

				for( size_t i=0; i<4u; ++i )
				{
					Ogre::Vector2 dirTo = cornerToCorner[i];

					const float dirLength = dirTo.normalise();

					const float cosAngle( dirTo.dotProduct( Ogre::Vector2::UNIT_X ) );

					if( dirLength < closestSiblingDistances[Borders::Right] &&
						cosAngle <= Ogre::Degree( 45.0f ).valueRadians() )
					{
						closestSiblings[Borders::Right] = widget2;
						closestSiblingDistances[Borders::Right] = dirLength;
					}

					if( dirLength < closestSiblingDistances[Borders::Left] &&
						cosAngle <= Ogre::Degree( 135.0f ).valueRadians() )
					{
						closestSiblings[Borders::Left] = widget2;
						closestSiblingDistances[Borders::Left] = dirLength;
					}

					if( cosAngle >= Ogre::Degree( 45.0f ).valueRadians() &&
						cosAngle <= Ogre::Degree( 135.0f ).valueRadians() )
					{
						float crossProduct = dirTo.crossProduct( Ogre::Vector2::UNIT_X );

						if( crossProduct <= 0.0f )
						{
							if( dirLength < closestSiblingDistances[Borders::Top] )
							{
								closestSiblings[Borders::Top] = widget2;
								closestSiblingDistances[Borders::Top] = dirLength;
							}
						}
						else
						{
							if( dirLength < closestSiblingDistances[Borders::Bottom] )
							{
								closestSiblings[Borders::Bottom] = widget2;
								closestSiblingDistances[Borders::Bottom] = dirLength;
							}
						}
					}
				}

				++it2;
			}

			for( size_t i=0; i<4u; ++i )
			{
				if( widget->m_autoSetNextWidget[i] )
					widget->setNextWidget( closestSiblings[i], static_cast<Borders::Borders>( i ) );
			}

			++itor;
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::autosetNavigation( Window *window )
	{
		if( window->m_widgetNavigationDirty )
		{
			autosetNavigation( window->m_children );
			window->m_widgetNavigationDirty = false;
		}

		if( window->m_windowNavigationDirty )
		{
			autosetNavigation( window->m_childWindows );
			window->m_windowNavigationDirty = false;
		}

		if( window->m_childrenNavigationDirty )
		{
			WindowVec::const_iterator itor = window->m_childWindows.begin();
			WindowVec::const_iterator end  = window->m_childWindows.end();

			while( itor != end )
				autosetNavigation( *itor++ );

			window->m_childrenNavigationDirty = false;
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::autosetNavigation()
	{
		checkVertexBufferCapacity();

		if( m_windowNavigationDirty )
		{
			autosetNavigation( m_windows );
			m_windowNavigationDirty = false;
		}

		if( m_childrenNavigationDirty )
		{
			WindowVec::const_iterator itor = m_windows.begin();
			WindowVec::const_iterator end  = m_windows.end();

			while( itor != end )
			{
				autosetNavigation( *itor++ );
				m_childrenNavigationDirty = false;
			}
		}
	}
	//-------------------------------------------------------------------------
	void CrystalManager::_setWindowNavigationDirty()
	{
		m_windowNavigationDirty = true;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::_notifyChildWindowIsDirty()
	{
		m_childrenNavigationDirty = true;
	}
	//-------------------------------------------------------------------------
	void CrystalManager::update()
	{
		autosetNavigation();
	}
	//-------------------------------------------------------------------------
	void CrystalManager::prepareRenderCommands()
	{
		Ogre::VertexBufferPacked *vertexBuffer = m_vao->getBaseVertexBuffer();

		UiVertex * RESTRICT_ALIAS vertex = reinterpret_cast<UiVertex * RESTRICT_ALIAS>(
											   vertexBuffer->map( 0, vertexBuffer->getNumElements() ) );
		UiVertex * RESTRICT_ALIAS startOffset = vertex;

		WindowVec::const_iterator itor = m_windows.begin();
		WindowVec::const_iterator end  = m_windows.end();

		while( itor != end )
		{
			vertex = (*itor)->fillBuffersAndCommands( vertex, Ogre::Vector2::ZERO,
													  Ogre::Matrix3::IDENTITY );
			++itor;
		}

		const size_t elementsWritten = vertex - startOffset;
		assert( elementsWritten <= vertexBuffer->getNumElements() );
		vertexBuffer->unmap( Ogre::UO_KEEP_PERSISTENT, 0u, elementsWritten );
	}
	//-------------------------------------------------------------------------
	void CrystalManager::render()
	{
		ApiEncapsulatedObjects apiObjects;

		Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();

		Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_UNLIT );

		apiObjects.lastHlmsCache = &c_dummyCache;

		Ogre::HlmsCache passCache = hlms->preparePassHash( 0, false, false, m_sceneManager );
		apiObjects.passCache = &passCache;
		apiObjects.hlms = hlms;
		apiObjects.lastVaoName = 0;
		apiObjects.commandBuffer = m_commandBuffer;
		apiObjects.indirectBuffer = m_indirectBuffer;
		apiObjects.indirectDraw = reinterpret_cast<uint8_t*>(
									  m_indirectBuffer->map( 0, m_indirectBuffer->getNumElements() ) );
		apiObjects.startIndirectDraw = apiObjects.indirectDraw;
		apiObjects.baseInstanceAndIndirectBuffers = 0;
		if( m_vaoManager->supportsIndirectBuffers() )
			apiObjects.baseInstanceAndIndirectBuffers = 2;
		else if( m_vaoManager->supportsBaseInstance() )
			apiObjects.baseInstanceAndIndirectBuffers = 1;
		apiObjects.vao = m_vao;
		apiObjects.drawCmd = 0;
		apiObjects.drawCountPtr = 0;
		apiObjects.primCount = 0;

		WindowVec::const_iterator itor = m_windows.begin();
		WindowVec::const_iterator end  = m_windows.end();

		while( itor != end )
		{
			(*itor)->addCommands( apiObjects );
			++itor;
		}

		m_indirectBuffer->unmap( Ogre::UO_KEEP_PERSISTENT );

		hlms->preCommandBufferExecution( m_commandBuffer );
		m_commandBuffer->execute();
		hlms->postCommandBufferExecution( m_commandBuffer );
	}
}
