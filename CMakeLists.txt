#This scripts will add all the cpp and h files under src and include folders, and
#assumes that your Ogre source code is in Dependencies/Ogre and that:
# In Windows you built Ogre into Dependencies/Ogre/build
# In Linux you built Release into Dependencies/Ogre/build/Release
# In Linux you built Debug into Dependencies/Ogre/build/Debug
# In MacOS you did the same as Linux, and didn't build Ogre as frameworks.
#
# If your source code is not at "Dependencies/Ogre"; you can use "mklink /D" to create
# a symbolic link to where the source code is located on Windows.
# On Linux, you can use "ln -s"

#If you get errors finding doxygen, i.e 'Could NOT find Doxygen (missing: dot)',
#you need to install graphviz, as well as doxygen
#apt install graphviz, brew install graphviz

#set( CMAKE_TOOLCHAIN_FILE CMake/iOS.cmake )

cmake_minimum_required( VERSION 3.3...4.0 )
project( ColibriGui )

set( COLIBRIGUI_FLEXIBILITY_LEVEL 0 CACHE STRING "\
	Higher flexibility levels convert some functions to virtual in \
	a tradeoff of flexibility for performance" )

if( ${CMAKE_VERSION} VERSION_GREATER 3.9 AND NOT COLIBRIGUI_LIB_ONLY )
	# We need to do this first, as OGRE.cmake will add another FindDoxygen.cmake file
	# which is older than the system-provided one.
	find_package(	Doxygen
					COMPONENTS dot
					OPTIONAL_COMPONENTS mscgen dia )
endif()

if( NOT COLIBRIGUI_LIB_ONLY )
	set( EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}" )

	include( CMake/Bootstrap.cmake )
	include( CMake/Dependencies/OGRE.cmake )
	include( CMake/Examples.cmake )
else()
	include( CMake/Bootstrap.cmake )
endif()

add_subdirectory( Dependencies )

if( NOT COLIBRIGUI_LIB_ONLY )
	setupOgre( OGRE_SOURCE, OGRE_BINARIES, OGRE_LIBRARIES, FALSE, FALSE )
else()
	include_directories( "${OGRE_SOURCE}/OgreMain/include" )
	include_directories( "${OGRE_BINARIES}/include" )
	include_directories( "${OGRE_SOURCE}/Components/Hlms/Common/include" )
	include_directories( "${OGRE_SOURCE}/Components/Hlms/Unlit/include" )
endif()

# Setup our application
include_directories( "./include" )
if( APPLE_IOS )
	include_directories( "${OGRE_SOURCE}/iOSDependencies/include/" )
elseif( ANDROID )
	include_directories( "${OGRE_SOURCE}/DependenciesAndroid/include" )
else()
	include_directories( "${OGRE_SOURCE}/Dependencies/include/" )
endif()
include_directories( "./Dependencies/icu/common" )
include_directories( ${ZLIB_INCLUDE_DIRS} )
include_directories( ${HARFBUZZ_INCLUDE_DIRS} )
include_directories( ${FREETYPE_INCLUDE_DIRS} )

include_directories( "./Dependencies/MSVC_Fix/2012" )
if( MSVC90 )
	include_directories( "./Dependencies/MSVC_Fix" )
endif()

if( COLIBRIGUI_FLEXIBILITY_LEVEL GREATER 0 )
	add_compile_definitions(COLIBRI_FLEXIBILITY_LEVEL=${COLIBRIGUI_FLEXIBILITY_LEVEL})
endif()

add_recursive( ./src/ColibriGui SOURCES )
add_recursive( ./include/ColibriGui HEADERS )
list( REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/./src/ColibriGui/iOS/Colibri_iOS.mm" )

if( APPLE )
	set( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11" )
	set( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++" )

	#Set the pluginFolder path in the plugins.cfg file.
	#Make a copy of the original one so it can be edited.
	execute_process( COMMAND "cp" "${OGRE_BINARIES}/bin/plugins.cfg" "/tmp/plugins.cfg" )
	execute_process( COMMAND bash -c "/usr/bin/sed -i '' 's#PluginFolder=#PluginFolder=${OGRE_BINARIES}/lib/macosx/${CMAKE_BUILD_TYPE}/#g' /tmp/plugins.cfg" )

	#file( GLOB_RECURSE RESOURCES ./src/*.storyboard )
	set( RESOURCES ${RESOURCES} ./bin/Data "./bin/Data/resources2.cfg" "/tmp/plugins.cfg" "${OGRE_SOURCE}/Samples/Media/" )


	if( APPLE_IOS )
		list( APPEND SOURCES
			"src/ColibriGui/iOS/Colibri_iOS.mm" )
		list( APPEND HEADERS
			"include/ColibriGui/iOS/Colibri_iOS.h" )
	endif()
endif()

add_library( ${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} )

target_link_libraries( ${PROJECT_NAME} icucommon ${HARFBUZZ_LIBRARIES} ${FREETYPE_LIBRARIES} ${ZLIB_LIBRARIES} sds_library )
target_link_libraries( ${PROJECT_NAME} ${OGRE_LIBRARIES} )

if( UNIX )
	target_link_libraries( ${PROJECT_NAME} dl )
endif()
if( NOT MSVC )
	set( ColibriWarningOptions -Wall -Winit-self -Wcast-qual -Wwrite-strings -Wextra
		-Wsign-conversion -Wconversion
		-Wno-unused-parameter -Wshadow -Wimplicit-fallthrough)
	if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
		set( ColibriWarningOptions ${compileOptions} -Winconsistent-missing-destructor-override -Wcomma )
	endif()
	target_compile_options( ${PROJECT_NAME} PRIVATE ${ColibriWarningOptions} )
endif()

if( APPLE )
	set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO" )
	set_target_properties( ${PROJECT_NAME} PROPERTIES RESOURCE "${RESOURCES}" )
	set_target_properties( ${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/OgreCommon/System/OSX/Resources/Info.plist )
	#set_target_properties( ${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE SampleBrowser_OSX.icns)
	#set( CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit -framework Metal -framework MetalKit -framework ModelIO" )
	#set( CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreGraphics -framework IOKit -framework CoreAudio -framework QuartzCore -framework Metal -framework MetalKit -framework ModelIO" )
	set( CMAKE_EXE_LINKER_FLAGS "
		-framework QuartzCore
		-framework Metal
		-framework OpenGL
	")
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

if( NOT COLIBRIGUI_LIB_ONLY )
	add_subdirectory( Examples/MainDemo )
	add_subdirectory( Examples/OffScreenCanvas2D )
	add_subdirectory( Examples/OffScreenCanvas3D )
endif()
