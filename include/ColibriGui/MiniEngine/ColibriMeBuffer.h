
#pragma once

#include "ColibriGui/MiniEngine/ColibriMiniEnginePrerequisites.h"

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	enum BufferType
	{
		/// Read access from GPU.
		/// i.e. Textures, most meshes.
		BT_IMMUTABLE,

		/** Read and write access from GPU. No access for CPU at all.
			i.e. RenderTextures, vertex buffers that will be used for R2VB
		@remarks
			Ogre will use staging buffers to upload and download contents,
			but don't do this frequently (except where needed,
			i.e. live video capture)
		*/
		BT_DEFAULT,

		/// Read access from GPU. Write access for CPU.
		/// i.e. Particles, dynamic textures. Dynamic buffers don't put a
		/// hidden buffer behind the scenes. You get what you ask, therefore it's
		/// your responsability to ensure you don't lock a region that is currently
		/// in use by the GPU (or else stall).
		BT_DYNAMIC_DEFAULT,

		/// Same as BT_DYNAMIC, but mapping will be persistent.
		BT_DYNAMIC_PERSISTENT,

		/// Same as BT_DYNAMIC_PERSISTENT, but mapping will be persistent and cache coherent.
		BT_DYNAMIC_PERSISTENT_COHERENT,
	};

	enum BufferBindableTypes
	{
		BBT_VERTEX = 1u << 0u,
		BBT_INDEX = 1u << 1u,
		BBT_CONST = 1u << 2u,
		BBT_TEX = 1u << 3u,
		BBT_UAV = 1u << 4u,
		BBT_INDIRECT = 1u << 5u
	};

	enum MappingState
	{
		MS_UNMAPPED,
		MS_MAPPED,
		NUM_MAPPING_STATE
	};

	enum UnmapOptions
	{
		/// Unmaps all types of mapping, including persistent buffers.
		UO_UNMAP_ALL,

		/// When unmapping, unmap() will keep persistent buffers mapped.
		/// Further calls to map will only do some error checking
		UO_KEEP_PERSISTENT
	};

	class MeBuffer
	{
	protected:
		size_t m_internalBufferStart;  /// In elements
		size_t m_finalBufferStart;     /// In elements, includes dynamic buffer frame offset
		size_t m_numElements;
		size_t m_bytesPerElement;
		size_t m_numElementsPadding;

		BufferType m_bufferType;
		uint16_t   m_bufferBindableTypes;

		MappingState       m_mappingState;
		MeBufferInterface *m_bufferInterface;

		/// Stores the range of the last map() call so that
		/// we can flush it correctly when calling unmap
		size_t m_lastMappingStart;
		size_t m_lastMappingCount;

	public:
		MeBuffer( size_t internalBufferStartBytes, size_t numElements, uint32_t bytesPerElement,
				  uint32_t numElementsPadding, BufferType bufferType, uint16_t bufferBindableTypes,
				  MeBufferInterface *bufferInterface );
		~MeBuffer();

		/** Unmaps or flushes the region mapped with MeBuffer::map.

			Alternatively, you can flush a smaller region
			(i.e. you didn't know which regions you were to update when mapping,
			but now that you're done, you know).

			The region being flushed is [flushStart; flushStart + flushSize)
		@param unmapOption
			When using persistent mapping, UO_KEEP_PERSISTENT will keep the map alive; but you will
			have to call map again to use it. This requirement allows Ogre to:
				1. Synchronize if needed (avoid mapping a region that is still in use)
				2. Emulate persistent mapping on Hardware/Drivers that don't support it.
		@param flushStartElem
			In elements, 0-based index (based on the mapped region) on where to start flushing from.
			Default is 0.
		@param flushSizeElem
			The length of the flushing region, which can't be bigger than 'elementCount' passed
			to @see map. When this value is 0, we flush until the end of the buffer starting
			from flushStartElem
		*/
		void unmap( UnmapOptions unmapOption, size_t flushStartElem = 0u, size_t flushSizeElem = 0u );

		/// Returns the mapping state. Note that if you call map with MS_PERSISTENT_INCOHERENT or
		/// MS_PERSISTENT_COHERENT, then call unmap( UO_KEEP_PERSISTENT ); the returned value will
		/// still be MS_PERSISTENT_INCOHERENT/_COHERENT when persistent mapping is supported.
		/// This differs from isCurrentlyMapped
		MappingState getMappingState() const { return m_mappingState; }
	};
}  // namespace Ogre

COLIBRIGUI_ASSUME_NONNULL_END
