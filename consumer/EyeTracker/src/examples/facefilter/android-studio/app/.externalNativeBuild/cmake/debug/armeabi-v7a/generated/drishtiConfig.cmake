

find_package(OpenCV REQUIRED)
find_package(ogles_gpgpu CONFIG REQUIRED) # review OpenGL dependencies
find_package(xgboost CONFIG REQUIRED)
find_package(acf CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED system filesystem)

##
## Header (INTERFACE) libraries
##

find_package(ARM_NEON_2_x86_SSE CONFIG REQUIRED) # header
find_package(Eigen3 CONFIG REQUIRED)             # header
find_package(cereal CONFIG REQUIRED)             # header
find_package(dlib CONFIG REQUIRED)               # header (for now)
find_package(half CONFIG REQUIRED)               # header
find_package(spdlog CONFIG REQUIRED)             # header
find_package(sse2neon CONFIG REQUIRED)           # header
if(ON)
  find_package(thread-pool-cpp CONFIG REQUIRED)  # header
endif()

####################################################
# Everything below here will typically not be used #
####################################################

if(ON)
  if(OFF)
    find_package(cvmatio CONFIG REQUIRED)
  endif()
endif()

##
## Experimental: typically disabled for release
##

if(OFF)
  find_package(dest CONFIG REQUIRED)
endif()

if(OFF)
  find_package(eos CONFIG REQUIRED)
  find_package(Boost CONFIG REQUIRED filesystem system)
endif()

if(ON)
  find_package(nlohmann_json CONFIG REQUIRED)
endif()

find_package(glm CONFIG REQUIRED)

if()
  find_package(imshow CONFIG REQUIRED) # debugging only
endif()

include("${CMAKE_CURRENT_LIST_DIR}/drishtiTargets.cmake")
check_required_components("drishtisdk")
