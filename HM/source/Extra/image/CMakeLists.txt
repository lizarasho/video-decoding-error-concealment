
# library
set( LIB_NAME ImageRotate )

set(OpenCV_DIR "D:\\opencv-master\\build\\install")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# get source files
file( GLOB SRC_FILES "*.cpp" )

# get include files
file( GLOB INC_FILES "*.h" )


# library
add_library( ${LIB_NAME} STATIC ${SRC_FILES} ${INC_FILES} )
target_compile_definitions( ${LIB_NAME} PUBLIC )


target_include_directories( ${LIB_NAME} PUBLIC . )
set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs)

# linking
target_link_libraries(${LIB_NAME}  ${OpenCV_LIBS})
# set the folder where to place the projects
set_target_properties( ${LIB_NAME} PROPERTIES FOLDER lib )
