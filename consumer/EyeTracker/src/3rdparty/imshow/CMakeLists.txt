cmake_minimum_required(VERSION 3.0)

include("cmake/HunterGate.cmake")
HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.18.18.tar.gz"
  SHA1 "ae1f72b9e8779a58f763179c77f609c0587d26cd"
  LOCAL
  )

project(imshow VERSION 1.0.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(IMSHOW_BUILD_EXAMPLES "Build examples" OFF)
option(IMSHOW_INSTALL "Perform installation" ON)
option(IMSHOW_WITH_OPENCV "Build with support for OpenCV" ON)

set(IMSHOW_3RDPARTY_LIBS "")

############
### glfw ###
############

hunter_add_package(glfw)
find_package(glfw3 REQUIRED)
list(APPEND IMSHOW_3RDPARTY_LIBS glfw)

##############
### OpenCV ###
##############

if(IMSHOW_WITH_OPENCV)
  hunter_add_package(OpenCV)
  find_package(OpenCV REQUIRED)
  list(APPEND IMSHOW_3RDPARTY_LIBS ${OpenCV_LIBS})
endif()

##############
### imshow ###
##############

add_library(imshow imshow/imshow.h imshow/imshow.cpp)
target_link_libraries(imshow PUBLIC ${IMSHOW_3RDPARTY_LIBS})
target_include_directories(imshow PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>"
  )

if(IMSHOW_WITH_OPENCV)
  target_compile_definitions(imshow PUBLIC IMSHOW_WITH_OPENCV)
endif()

### ############
### Examples ###
### ############

if(IMSHOW_BUILD_EXAMPLES)

  # standard test:
  add_executable(imshow-test examples/main.cpp)
  target_link_libraries(imshow-test PUBLIC ${OpenCV_LIBS} imshow)
  if(IMSHOW_INSTALL)
    install(TARGETS imshow-test DESTINATION bin)
  endif()

  if(IMSHOW_WITH_OPENCV)
    # opencv interface test:
    add_executable(imshow-test-cv examples/cvtest.cpp)
    target_link_libraries(imshow-test-cv PUBLIC imshow ${IMSHOW_3RDPARTY_LIBS})
    if(IMSHOW_INSTALL)
      install(TARGETS imshow-test-cv DESTINATION bin)
    endif()
  endif()

endif()

###############
### Install ###
###############

if(NOT IMSHOW_INSTALL)
  return()
endif()

### Install ###
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")

set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Note: use 'targets_export_name'
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${project_config}"
     INSTALL_DESTINATION "${config_install_dir}"
)

install(
    TARGETS imshow
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/imshow/" # Trailing slash is required
  DESTINATION "${include_install_dir}/${PROJECT_NAME}"
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
  )

install(
  FILES "${project_config}" "${version_config}"
  DESTINATION "${config_install_dir}"
  )

install(
  EXPORT "${targets_export_name}"
  NAMESPACE "${namespace}"
  DESTINATION "${config_install_dir}"
  )
