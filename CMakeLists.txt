PROJECT(user_image_annotator)
cmake_minimum_required(VERSION 2.6)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE ON)

FIND_PACKAGE( OpenCV REQUIRED )
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
CONFIGURE_FILE("${PROJECT_SOURCE_DIR}/contour_image_annotator_path.h.in"
               "${PROJECT_BINARY_DIR}/contour_image_annotator_path.h")
OPTION(USE_PCL_FOR_GROUND_PLANE 0)
OPTION(USE_IMAGEMAGICK_FOR_16_COLORS_CONVERSION 0)

IF(USE_PCL_FOR_GROUND_PLANE)
  find_package(PCL 1.3 REQUIRED)
  include_directories(${PCL_INCLUDE_DIRS})
  link_directories(${PCL_LIBRARY_DIRS})
  add_definitions(${PCL_DEFINITIONS})
ENDIF(USE_PCL_FOR_GROUND_PLANE)

IF(USE_IMAGEMAGICK_FOR_16_COLORS_CONVERSION)
  FIND_PACKAGE(ImageMagick COMPONENTS convert REQUIRED)
ENDIF(USE_IMAGEMAGICK_FOR_16_COLORS_CONVERSION)


ADD_EXECUTABLE(contour_image_annotator contour_image_annotator.cpp
                                    contour_image_annotator.h
                                    exec_system_get_output.h
                                    convert_n_colors.h
                                    cv_conversion_float_uchar.h
                                    min_max.h
                                    nan_handling.h)
TARGET_LINK_LIBRARIES( contour_image_annotator ${OpenCV_LIBS})

ADD_EXECUTABLE(clean_user_image           clean_user_image.cpp)
TARGET_LINK_LIBRARIES( clean_user_image   ${OpenCV_LIBS})

ADD_EXECUTABLE(user_image_annotator user_image_annotator.cpp
                                    contour_image_annotator.h
                                    depth_canny.h
                                    ground_plane_finder.h)
TARGET_LINK_LIBRARIES( user_image_annotator ${OpenCV_LIBS})
IF(USE_PCL_FOR_GROUND_PLANE)
  TARGET_LINK_LIBRARIES( user_image_annotator ${PCL_LIBRARIES})
ENDIF(USE_PCL_FOR_GROUND_PLANE)


