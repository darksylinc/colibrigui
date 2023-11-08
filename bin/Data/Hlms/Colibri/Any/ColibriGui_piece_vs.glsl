@property( colibri_gui )

@piece( custom_vs_attributes )
	@property( ogre_version < 2003000 )
		#define vulkan_layout(x)
	@end

	vulkan_layout( OGRE_NORMAL ) in float4 normal;

	@property( colibri_text )
		vulkan_layout( OGRE_TANGENT ) in uint tangent;
		vulkan_layout( OGRE_BLENDINDICES ) in uint2 blendIndices;
	@end
@end

@piece( custom_vs_preExecution )
	@property( !colibri_text )
		uint colibriDrawId = inVs_drawId
		@property( !colibri_custom_shape )
				+ ((uint(inVs_vertexId) - worldMaterialIdx[inVs_drawId].w) / 54u)
		@end
				;
		#undef finalDrawId
		#define finalDrawId colibriDrawId
	@end

	#define worldViewProj 1.0f

	@property( hlms_pso_clip_distances >= 4 )
		gl_ClipDistance[0] = normal.x;
		gl_ClipDistance[1] = normal.y;
		gl_ClipDistance[2] = normal.z;
		gl_ClipDistance[3] = normal.w;
	@else
		outVs.emulatedClipDistance = normal;
	@end

	@property( colibri_text )
		uint vertId = (uint(inVs_vertexId) - worldMaterialIdx[inVs_drawId].w) % 6u;
		outVs.uvText.x = (vertId <= 1u || vertId == 5u) ? 0.0f : float( blendIndices.x );
		outVs.uvText.y = (vertId == 0u || vertId >= 4u) ? 0.0f : float( blendIndices.y );
		outVs.pixelsPerRow		= blendIndices.x;
		outVs.glyphOffsetStart	= tangent;
	@end
@end

@end
