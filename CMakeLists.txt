#This scripts will add all the cpp and h files under src and include folders, and
#assumes that your Ogre source code is in Dependencies/Ogre and that:
# In Windows you built Ogre into Dependencies/Ogre/build
# In Linux you built Release into Dependencies/Ogre/build/Release
# In Linux you built Debug into Dependencies/Ogre/build/Debug
#
# If your source code is not at "Dependencies/Ogre"; you can use "mklink /D" to create
# a symbolic link to where the source code is located on Windows.
# On Linux, you can use "ln -s"

#set( CMAKE_TOOLCHAIN_FILE CMake/iOS.cmake )

cmake_minimum_required( VERSION 3.3 )
project( CrystalGui )

if( ${CMAKE_VERSION} VERSION_GREATER 3.9 )
	# We need to do this first, as OGRE.cmake will add another FindDoxygen.cmake file
	# which is older than the system-provided one.
	find_package(	Doxygen
					REQUIRED dot
					OPTIONAL_COMPONENTS mscgen dia )
endif()

set( EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}" )

include( CMake/Bootstrap.cmake )
include( CMake/Dependencies/OGRE.cmake )

add_subdirectory( Dependencies )

setupOgre( OGRE_SOURCE, OGRE_BINARIES, OGRE_LIBRARIES )

# Setup our application
include_directories( "./include" )
include_directories( "./Dependencies/Ogre/Dependencies/include/" )
include_directories( "./Dependencies/icu/common" )
include_directories( ${ZLIB_INCLUDE_DIRS} )
include_directories( ${HARFBUZZ_INCLUDE_DIRS} )
include_directories( ${FREETYPE_INCLUDE_DIRS} )

include_directories( "./Dependencies/MSVC_Fix/2012" )
if( MSVC90 )
	include_directories( "./Dependencies/MSVC_Fix" )
endif()

add_recursive( ./src SOURCES )
add_recursive( ./include HEADERS )

if( APPLE )
	file( GLOB_RECURSE RESOURCES ./src/*.storyboard )
	set( RESOURCES ${RESOURCES} ./Data/Resources.cfg ./bin/Data )
endif()

add_executable( ${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${RESOURCES} )

target_link_libraries( ${PROJECT_NAME} icucommon ${HARFBUZZ_LIBRARIES} ${FREETYPE_LIBRARIES} ${ZLIB_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} ${OGRE_LIBRARIES} )

if( UNIX )
	target_link_libraries( ${PROJECT_NAME} dl )
endif()

if( APPLE )
	set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO" )
	set_target_properties( ${PROJECT_NAME} PROPERTIES RESOURCE "${RESOURCES}" )
	set_target_properties( ${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist )
	#set_target_properties( ${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE SampleBrowser_OSX.icns)
	set( CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit -framework Metal -framework MetalKit -framework ModelIO" )
endif()

if( ${CMAKE_VERSION} VERSION_GREATER 3.9 )
	if( DOXYGEN_FOUND )
		set( DOXYGEN_EXTRACT_ALL NO )
		set( DOXYGEN_EXTRACT_PRIVATE YES )
		set( DOXYGEN_EXTRACT_PACKAGE YES )
		set( DOXYGEN_EXTRACT_STATIC YES )
		set( DOXYGEN_EXTRACT_LOCAL_METHODS YES )
		doxygen_add_docs( doxygen ${SOURCES} ${HEADERS} )
	endif()
endif()
