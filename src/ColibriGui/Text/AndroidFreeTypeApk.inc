
#include "sds/sds_fstreamApk.h"

#include "sds/sds_fstreamApk.h"

#include <android/asset_manager.h>

namespace Colibri
{
	FT_CALLBACK_DEF( unsigned long )
	FtAndroidStreamRead( FT_Stream stream, unsigned long offset, unsigned char *buffer,
						 unsigned long count )
	{
		if( !count && offset > stream->size )
			return 1;

		AAsset *asset = reinterpret_cast<AAsset *>( stream->descriptor.pointer );

		if( stream->pos != offset )
			AAsset_seek( asset, static_cast<off_t>( offset ), SEEK_SET );

		const int bytesRead = std::max( AAsset_read( asset, buffer, count ), 0 );
		return static_cast<unsigned long>( bytesRead );
	}

	FT_CALLBACK_DEF( void )
	FtAndroidStreamClose( FT_Stream stream )
	{
		AAsset *asset = reinterpret_cast<AAsset *>( stream->descriptor.pointer );
		AAsset_close( asset );

		stream->descriptor.pointer = NULL;
		stream->size = 0;
		stream->base = NULL;

		delete stream;
	}
}  // namespace Colibri
