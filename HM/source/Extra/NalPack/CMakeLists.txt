set( LIB_NAME NalPack )
# get source files
file( GLOB SRC_FILES "NalPack.cpp" )

# get include files
file( GLOB INC_FILES "NalPack.h" )


add_library( ${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES})
target_compile_definitions( ${LIB_NAME} PUBLIC )

# set the folder where to place the projects
set_target_properties( ${LIB_NAME} PROPERTIES FOLDER lib )
