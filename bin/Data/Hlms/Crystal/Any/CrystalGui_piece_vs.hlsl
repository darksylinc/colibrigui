@property( crystal_gui )

@piece( custom_vs_attributes )
	float4 normal : NORMAL;

	@property( crystal_text )
		uint tangent : TANGENT;
		uint2 blendIndices : BLENDINDICES;
	@end

	uint vertexId : SV_VertexID;
	#define gl_VertexID input.vertexId
@end

@piece( custom_vs_preExecution )
	@property( !crystal_text )
		uint crystalDrawId = input.drawId + (uint(gl_VertexID) / 54u);
	@end @property( crystal_text )
		uint crystalDrawId = input.drawId + (uint(gl_VertexID) / 6u);
	@end
	#undef finalDrawId
	#define finalDrawId crystalDrawId
	#define worldViewProj 1.0f

	outVs.gl_ClipDistance0[0] = input.normal.x;
	outVs.gl_ClipDistance0[1] = input.normal.y;
	outVs.gl_ClipDistance0[2] = input.normal.z;
	outVs.gl_ClipDistance0[3] = input.normal.w;

	@property( crystal_text )
		uint vertId = uint(gl_VertexID) % 6u;
		outVs.uvText.x = (vertId <= 1u || vertId == 5u) ? 0.0f : float( input.blendIndices.x );
		outVs.uvText.y = (vertId == 0u || vertId >= 4u) ? 0.0f : float( input.blendIndices.y );
		outVs.pixelsPerRow		= input.blendIndices.x;
		outVs.glyphOffsetStart	= input.tangent;
	@end
@end

@end
