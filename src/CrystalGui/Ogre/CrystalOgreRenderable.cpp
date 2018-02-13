
#include "CrystalGui/Ogre/CrystalOgreRenderable.h"

#include "OgreSceneManager.h"

namespace Ogre
{
	CrystalOgreRenderable::CrystalOgreRenderable( IdType id, ObjectMemoryManager *objectMemoryManager,
												  SceneManager *manager, uint8 renderQueueId,
												  IndexBufferPacked *indexBuffer,
												  bool ownsVao ) :
		MovableObject( id, objectMemoryManager, manager, renderQueueId ),
		Renderable()
	{
		//Set the bounds!!! Very important! If you don't set it, the object will not
		//appear on screen as it will always fail the frustum culling.
		//This example uses an infinite aabb; but you really want to use an Aabb as tight
		//as possible for maximum efficiency (so Ogre avoids rendering an object that
		//is off-screen)
		//Note the WorldAabb and the WorldRadius will be automatically updated by Ogre
		//every frame as rendering begins (it's calculated based on the local version
		//combined with the scene node's transform).
		Aabb aabb( Aabb::BOX_INFINITE );
		mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
		mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
		mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Real>::max();
		mObjectData.mWorldRadius[mObjectData.mIndex] = std::numeric_limits<Real>::max();

		if( ownsVao )
			createBuffers( indexBuffer );

		//This is very important!!! A MovableObject must tell what Renderables to render
		//through this array. Since we derive from both MovableObject & Renderable, add
		//ourselves to the array. Otherwise, nothing will be rendered.
		//Tip: You can use this array as a rough way to show or hide Renderables
		//that belong to this MovableObject.
		mRenderables.push_back( this );

		//If we don't set a datablock, we'll crash Ogre.
//		this->setDatablock( Root::getSingleton().getHlmsManager()->
//							getHlms( HLMS_UNLIT )->getDefaultDatablock() );
	}
	//-----------------------------------------------------------------------------------
	CrystalOgreRenderable::~CrystalOgreRenderable()
	{
		assert( mVaoPerLod[VpNormal].empty() && "destroyBuffers not called!" );
	}
	//-----------------------------------------------------------------------------------
	Ogre::IndexBufferPacked* CrystalOgreRenderable::createIndexBuffer( VaoManager *vaoManager )
	{
		//6 indices per quad (3 indices per triangle)
		//3x3 grid = 9 quads => 6 x 9 indices per widget.
		//Vertices are expected to be layed out like this:
		//	 0, 1,		 2, 3,
		//	 4, 5,		 6, 7,
		//
		//
		//	 8, 9,		10,11,
		//	12,13		14,15
		//
		//	x-x------x-x
		//	| |		 | |
		//	x-x------x-x
		//	| |      | |
		//	| |      | |
		//	| |      | |
		//	x-x------x-x
		//	| |		 | |
		//	x-x------x-x
		// maxVerticesPerBuffer = 4096; which means we support up to 4096 widgets per Window.
		// And this precomputed buffer requires 432kb
		const size_t verticesPerQuad		= 16u;
		const size_t maxVerticesPerBuffer	= 65536u / verticesPerQuad;
		const size_t numIndices = maxVerticesPerBuffer * 6u * 9u;
		uint16 *indices = reinterpret_cast<uint16*>( OGRE_MALLOC_SIMD( sizeof(uint16) * numIndices,
																	   MEMCATEGORY_GEOMETRY ) );
		for( size_t i=0; i<numIndices; i += (6u * 9u) )
		{
			//Perform top, then center, then bottom rows
			for( size_t j=0; j<3u; ++j )
			{
				const size_t dstIdx = i + j * 18u;
				const size_t srcIdx = i + j * 4u;
				//Left column's quad
				indices[dstIdx +  0u] = srcIdx + 0u;
				indices[dstIdx +  1u] = srcIdx + 4u;
				indices[dstIdx +  2u] = srcIdx + 5u;

				indices[dstIdx +  3u] = srcIdx + 5u;
				indices[dstIdx +  4u] = srcIdx + 1u;
				indices[dstIdx +  5u] = srcIdx + 0u;

				//Center column's quad
				indices[dstIdx +  6u] = srcIdx + 1u;
				indices[dstIdx +  7u] = srcIdx + 5u;
				indices[dstIdx +  8u] = srcIdx + 6u;

				indices[dstIdx +  9u] = srcIdx + 6u;
				indices[dstIdx + 10u] = srcIdx + 2u;
				indices[dstIdx + 11u] = srcIdx + 1u;

				//Right column's quad
				indices[dstIdx + 12u] = srcIdx + 2u;
				indices[dstIdx + 13u] = srcIdx + 6u;
				indices[dstIdx + 14u] = srcIdx + 7u;

				indices[dstIdx + 15u] = srcIdx + 7u;
				indices[dstIdx + 16u] = srcIdx + 3u;
				indices[dstIdx + 17u] = srcIdx + 2u;
			}
		}

		IndexBufferPacked *indexBuffer = 0;

		try
		{
			indexBuffer = vaoManager->createIndexBuffer( IndexBufferPacked::IT_16BIT,
														 numIndices, BT_IMMUTABLE,
														 indices, true );
		}
		catch( Exception &e )
		{
			OGRE_FREE_SIMD( indexBuffer, MEMCATEGORY_GEOMETRY );
			indexBuffer = 0;
			throw e;
		}

		return indexBuffer;
	}
	//-----------------------------------------------------------------------------------
	void CrystalOgreRenderable::createBuffers( Ogre::IndexBufferPacked *indexBuffer )
	{
		VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

		//Vertex declaration
		VertexElement2Vec vertexElements;
		vertexElements.push_back( VertexElement2( VET_FLOAT2, VES_POSITION ) );
		vertexElements.push_back( VertexElement2( VET_USHORT2_NORM, VES_TEXTURE_COORDINATES ) );
		vertexElements.push_back( VertexElement2( VET_UBYTE4_NORM, VES_DIFFUSE ) );
		vertexElements.push_back( VertexElement2( VET_FLOAT4, VES_NORMAL ) );

		Ogre::VertexBufferPacked *vertexBuffer = 0;
		try
		{
			//Create the actual vertex buffer.
			vertexBuffer = vaoManager->createVertexBuffer( vertexElements, 8,
														   BT_DYNAMIC_PERSISTENT,
														   0, false );
		}
		catch( Ogre::Exception &e )
		{
			OGRE_FREE_SIMD( vertexBuffer, Ogre::MEMCATEGORY_GEOMETRY );
			vertexBuffer = 0;
			throw e;
		}

		VertexBufferPackedVec vertexBuffers;
		vertexBuffers.push_back( vertexBuffer );
		Ogre::VertexArrayObject *vao = vaoManager->createVertexArrayObject(
					vertexBuffers, indexBuffer, OT_TRIANGLE_LIST );

		mVaoPerLod[0].push_back( vao );
		mVaoPerLod[1].push_back( vao );
	}
	//-----------------------------------------------------------------------------------
	void CrystalOgreRenderable::getSharedBuffersFromParent( CrystalOgreRenderable *parent )
	{
		assert( mVaoPerLod[0].empty() && mVaoPerLod[1].empty() &&
				"getBuffersFromParentWindow or createBuffers already called!" );
		mVaoPerLod[0] = parent->mVaoPerLod[0];
		mVaoPerLod[1] = parent->mVaoPerLod[1];
	}
	//-----------------------------------------------------------------------------------
	void CrystalOgreRenderable::destroyBuffers( bool ownsVao )
	{
		if( ownsVao )
		{
			VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

			VertexArrayObjectArray::const_iterator itor = mVaoPerLod[0].begin();
			VertexArrayObjectArray::const_iterator end  = mVaoPerLod[0].end();
			while( itor != end )
			{
				VertexArrayObject *vao = *itor;

				const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
				VertexBufferPackedVec::const_iterator itBuffers = vertexBuffers.begin();
				VertexBufferPackedVec::const_iterator enBuffers = vertexBuffers.end();

				while( itBuffers != enBuffers )
				{
					vaoManager->destroyVertexBuffer( *itBuffers );
					++itBuffers;
				}

				/* Do not destroy the index buffer. We do not own it.
				if( vao->getIndexBuffer() )
					vaoManager->destroyIndexBuffer( vao->getIndexBuffer() );*/
				vaoManager->destroyVertexArrayObject( vao );

				++itor;
			}
		}

		mVaoPerLod[VpNormal].clear();
		mVaoPerLod[VpShadow].clear();
	}
	//-----------------------------------------------------------------------------------
	const String& CrystalOgreRenderable::getMovableType(void) const
	{
		return BLANKSTRING;
	}
	//-----------------------------------------------------------------------------------
	const LightList& CrystalOgreRenderable::getLights(void) const
	{
		return this->queryLights(); //Return the data from our MovableObject base class.
	}
	//-----------------------------------------------------------------------------------
	void CrystalOgreRenderable::getRenderOperation( v1::RenderOperation& op , bool casterPass )
	{
		OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
						"CrystalOgreRenderable do not implement getRenderOperation."
						" You've put a v2 object in "
						"the wrong RenderQueue ID (which is set to be compatible with "
						"v1::Entity). Do not mix v2 and v1 objects",
						"CrystalOgreRenderable::getRenderOperation" );
	}
	//-----------------------------------------------------------------------------------
	void CrystalOgreRenderable::getWorldTransforms( Matrix4* xform ) const
	{
		OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
						"CrystalOgreRenderable do not implement getWorldTransforms."
						" You've put a v2 object in "
						"the wrong RenderQueue ID (which is set to be compatible with "
						"v1::Entity). Do not mix v2 and v1 objects",
						"CrystalOgreRenderable::getRenderOperation" );
	}
	//-----------------------------------------------------------------------------------
	bool CrystalOgreRenderable::getCastsShadows(void) const
	{
		OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
						"CrystalOgreRenderable do not implement getCastsShadows."
						" You've put a v2 object in "
						"the wrong RenderQueue ID (which is set to be compatible with "
						"v1::Entity). Do not mix v2 and v1 objects",
						"CrystalOgreRenderable::getRenderOperation" );
	}
}
