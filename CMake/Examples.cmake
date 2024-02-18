function( getCommonFiles RESULT_FILES INCLUDE_FOLDERS )
	# add_recursive( ./src SOURCES )
	# add_recursive( ./include HEADERS )

	# add_recursive( ./src/OgreCommon/System/OSX/*.mm SOURCES )

	# add_recursive would have added all .mm files, including the ios ones.
	# We need to be able to set specifics.
	file( GLOB_RECURSE SOURCES
		"${CMAKE_SOURCE_DIR}/src/OgreCommon/*.cpp"
		"${CMAKE_SOURCE_DIR}/include/OgreCommon/*.h"
		"${CMAKE_SOURCE_DIR}/Examples/Common/*.cpp"
		"${CMAKE_SOURCE_DIR}/Examples/Common/*.h" )
	if( APPLE )
		file( GLOB_RECURSE MMSOURCES "${CMAKE_SOURCE_DIR}/src/OgreCommon/System/OSX/*.mm" )
	endif()

	set( ${RESULT_FILES} ${SOURCES} ${MMSOURCES} PARENT_SCOPE )
	set( ${INCLUDE_FOLDERS} "${CMAKE_SOURCE_DIR}/Examples/Common" PARENT_SCOPE )
endfunction()
