
#pragma once

#include "ColibriGui/MiniEngine/ColibriMiniEnginePrerequisites.h"

#include <vector>

COLIBRIGUI_ASSUME_NONNULL_BEGIN

namespace Ogre
{
	namespace MeVertexSemantic
	{
		enum MeVertexSemantic
		{
			Position,
			Normal,
			Diffuse,
			Indices,
			TexCoord,
			Tangent,
		};
	}

	enum MeVertexFormat
	{
		MVF_FLOAT2,
		MVF_FLOAT4,
		MVF_USHORT2_UNORM,
		MVF_UBYTE4_UNORM,
		MVF_UINT1
	};

	struct MeVertexElement
	{
		MeVertexFormat                     m_type;
		MeVertexSemantic::MeVertexSemantic m_semantic;

		MeVertexElement( MeVertexFormat type, MeVertexSemantic::MeVertexSemantic semantic ) :
			m_type( type ),
			m_semantic( semantic )
		{
		}

		bool operator==( const MeVertexElement _r ) const
		{
			return m_type == _r.m_type && m_semantic == _r.m_semantic;
		}

		bool operator==( MeVertexSemantic::MeVertexSemantic semantic ) const
		{
			return m_semantic == semantic;
		}

		/// Warning: Beware a MeVertexElementVec shouldn't be sorted.
		/// The order in which they're pushed into the vector defines the offsets
		/// in the vertex buffer. Altering the order would cause the GPU to
		/// to read garbage when trying to interpret the vertex buffer
		/// This operator exists because it's useful when implementing
		/// a '<' operator in other structs that contain MeVertexElementVecs
		/// (see HlmsPso)
		bool operator<( const MeVertexElement &_r ) const
		{
			// clang-format off
			if( this->m_type < _r.m_type ) return true;
			if( this->m_type > _r.m_type ) return false;
			// clang-format on

			return this->m_semantic < _r.m_semantic;
		}
	};

	typedef std::vector<MeVertexElement> MeVertexElementVec;
}  // namespace Ogre

COLIBRIGUI_ASSUME_NONNULL_END
