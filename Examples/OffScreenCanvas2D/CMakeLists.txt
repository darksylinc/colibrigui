set( SAMPLE_NAME Example_OffScreenCanvas2D )

getCommonFiles( OGRE_COMMON_FILES OGRE_COMMON_INCLUDE )
add_recursive( ./ SOURCE_FILES )

add_executable( ${SAMPLE_NAME} WIN32 MACOSX_BUNDLE ${OGRE_COMMON_FILES} ${SOURCE_FILES} )

target_link_libraries( ${SAMPLE_NAME} ColibriGui )
target_include_directories( ${SAMPLE_NAME} PUBLIC ${OGRE_COMMON_INCLUDE} )
