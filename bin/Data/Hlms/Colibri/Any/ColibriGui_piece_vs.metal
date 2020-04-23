@property( colibri_gui )

@piece( custom_vs_attributes )
	float4 normal [[attribute(VES_NORMAL)]];

	@property( colibri_text )
		uint tangent [[attribute(VES_TANGENT)]];
		uint2 blendIndices [[attribute(VES_BLEND_INDICES)]];
	@end
@end

@piece( custom_vs_uniformDeclaration )
	, uint gl_VertexID	[[vertex_id]]
@end

@piece( custom_vs_preExecution )
	@property( !colibri_text )
		uint colibriDrawId = drawId + ((uint(gl_VertexID) - worldMaterialIdx[drawId].w) / 54u);
		#undef finalDrawId
		#define finalDrawId colibriDrawId
	@end

	#define worldViewProj 1.0f

	outVs.gl_ClipDistance0[0] = input.normal.x;
	outVs.gl_ClipDistance0[1] = input.normal.y;
	outVs.gl_ClipDistance0[2] = input.normal.z;
	outVs.gl_ClipDistance0[3] = input.normal.w;

	@property( colibri_text )
		uint vertId = (uint(gl_VertexID) - worldMaterialIdx[drawId].w) % 6u;
		outVs.uvText.x = (vertId <= 1u || vertId == 5u) ? 0.0f : float( input.blendIndices.x );
		outVs.uvText.y = (vertId == 0u || vertId >= 4u) ? 0.0f : float( input.blendIndices.y );
		outVs.pixelsPerRow		= input.blendIndices.x;
		outVs.glyphOffsetStart	= input.tangent;
	@end
@end

@end
