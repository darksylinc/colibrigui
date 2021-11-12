cmake_minimum_required( VERSION 3.5 )

if( NOT COLIBRIGUI_LIB_ONLY )
	set( EXTRA_FOLDER "" )
	if( ANDROID )
		set( EXTRA_FOLDER "Android/" )
	endif()

	# Works on Windows
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build_result/${EXTRA_FOLDER}lib/${CMAKE_BUILD_TYPE}" CACHE
		PATH "Directory where all the .lib files are dumped." FORCE )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY
		"${CMAKE_CURRENT_SOURCE_DIR}/../bin/${EXTRA_FOLDER}${CMAKE_BUILD_TYPE}" CACHE
		PATH "Directory where .exe and .dll files are dumped." FORCE )
endif()

# Works on Linux
set( EXECUTABLE_OUTPUT_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" )
set( LIBRARY_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )

add_subdirectory( icu )

set( SKIP_INSTALL_ALL YES )
add_subdirectory( zlib )
set( ZLIB_FOUND YES CACHE INTERNAL "" FORCE )
if( ANDROID OR APPLE_IOS )
	set( ZLIB_LIBRARY zlibstatic CACHE INTERNAL "" FORCE )
	set( ZLIB_LIBRARIES zlibstatic CACHE INTERNAL "" FORCE )
else()
	set( ZLIB_LIBRARY zlib CACHE INTERNAL "" FORCE )
	set( ZLIB_LIBRARIES zlib CACHE INTERNAL "" FORCE )
endif()
set( ZLIB_INCLUDE_DIRS
	${PROJECT_BINARY_DIR}/Dependencies/zlib
	${CMAKE_CURRENT_SOURCE_DIR}/zlib CACHE INTERNAL "" FORCE )

# Setup Harfbuzz <-> Freetype2 interoperability.
# Freetype depends on Harfbuzz to provide better auto-hinting (optional). However,
# in order for this to work, Harfbuzz defines a few interop functions and for that,
# it depends on Freetype.
# In other words Freetype depends on Harfbuzz, Harfbuzz depends on Freetype.
# It's a chicken and egg problem, which happens to work as long as both libs
# are static (or if compiled together, then compiled as a single dll, or
# compiled multiple times iteratively until both dependencies are met).
set( HB_HAVE_FREETYPE YES )
set( FREETYPE_FOUND YES CACHE INTERNAL "" FORCE )
set( FREETYPE_LIBRARY freetype CACHE INTERNAL "" FORCE )
set( FREETYPE_LIBRARIES freetype CACHE INTERNAL "" FORCE )
set( FREETYPE_INCLUDE_DIRS
	${PROJECT_BINARY_DIR}/Dependencies/freetype2/include/freetype/config
	${CMAKE_CURRENT_SOURCE_DIR}/freetype2/include CACHE INTERNAL "" FORCE )
add_subdirectory( harfbuzz )
set( HARFBUZZ_FOUND YES CACHE INTERNAL "" FORCE )
set( HARFBUZZ_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/harfbuzz/src CACHE INTERNAL "" FORCE )
set( HARFBUZZ_LIBRARY harfbuzz CACHE INTERNAL "" FORCE )
set( HARFBUZZ_LIBRARIES harfbuzz CACHE INTERNAL "" FORCE )

set( DISABLE_FORCE_DEBUG_POSTFIX 1 )
set( WITH_ZLIB OFF )
set( WITH_BZip2 OFF )
set( WITH_PNG OFF )
set( WITH_HarfBuzz OFF )
set( SKIP_INSTALL_ALL YES )
add_subdirectory( freetype2 )

add_subdirectory( sds_library )
