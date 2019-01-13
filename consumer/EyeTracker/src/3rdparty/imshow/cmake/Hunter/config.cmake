# Copyright (c) 2017, David Hirvonen
# All rights reserved.

if(APPLE)
  set(OPENCV_CMAKE_ARGS
    WITH_CUDA=OFF
    )
endif()

hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS "${OPENCV_CMAKE_ARGS}")
