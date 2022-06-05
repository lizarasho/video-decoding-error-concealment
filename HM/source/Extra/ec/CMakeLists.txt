
# library
set( LIB_NAME Repairer )

# get source files
file( GLOB SRC_FILES "*.cpp" )

# get include files
file( GLOB INC_FILES "*.h" )


# library
add_library( ${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES} )
target_compile_definitions( ${LIB_NAME} PUBLIC )


target_include_directories( ${LIB_NAME} PUBLIC . )
target_link_libraries( ${LIB_NAME} TLibCommon Threads::Threads ImageRotate )

# set the folder where to place the projects
set_target_properties( ${LIB_NAME} PROPERTIES FOLDER lib )
