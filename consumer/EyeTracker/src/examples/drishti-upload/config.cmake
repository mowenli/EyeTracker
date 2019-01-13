message(">:>:>:>:>:>:>:>:>:>:>:>:>:>:>:>:>: ${CMAKE_MODULE_PATH} <:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:<:")

### Toggle OpenGL ES 3.0 ###
if(DRISHTI_OPENGL_ES3)
  set(use_opengl_es3 ON)
else()
  set(use_opengl_es3 OFF)
endif()

string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

message("-- config.cmake --")
message("  MSVC: ${MSVC}")
message("  APPLE: ${APPLE}")
message("  ANDROID: ${ANDROID}")
message("  IOS: ${IOS}")
message("  is_linux: ${is_linux}")
message("  XCODE: ${XCODE}")

option(DRISHTI_BUILD_OPENCV_WORLD "Build OpenCV world (monolithic lib)" ON)

set(OPENCV_CMAKE_ARGS
  #### Repeat HUNTER ARGS ###
  # * https://github.com/ruslo/hunter/blob/master/cmake/projects/OpenCV/hunter.cmake
  # NOTE: In general there is no need to specify default CMake arguments of
  # package, they will be inherited.
  BUILD_ANDROID_EXAMPLES=OFF
  BUILD_DOCS=OFF
  BUILD_EXAMPLES=OFF
  BUILD_PERF_TESTS=OFF
  BUILD_TESTS=OFF
  BUILD_opencv_apps=OFF
  BUILD_ZLIB=OFF
  BUILD_TIFF=OFF
  BUILD_PNG=OFF
  BUILD_opencv_python2=OFF

  ### Custom ARGS ###
  BUILD_ANDROID_SERVICE=OFF
  ANDROID_EXAMPLES_WITH_LIBS=OFF    # "Build binaries of Android examples with native libraries"
  BUILD_opencv_world=${DRISHTI_BUILD_OPENCV_WORLD}
  BUILD_EIGEN=OFF  ### for convenient linking
  BUILD_LIST=core,imgproc,videoio,highgui,videostab,objdetect

  ### Custom ARGS (WITH_*) ###
  WITH_1394=OFF           # "Include IEEE1394 support"
  WITH_CARBON=OFF         # "Use Carbon for UI instead of Cocoa"
  WITH_CLP=OFF            # "Include Clp support (EPL)"
  WITH_CSTRIPES=OFF       # "Include C= support"
  WITH_CUBLAS=OFF         # "Include NVidia Cuda Basic Linear Algebra Subprograms (BLAS) library support"
  WITH_CUDA=OFF           # "Include NVidia Cuda Runtime support"
  WITH_CUFFT=OFF          # "Include NVidia Cuda Fast Fourier Transform (FFT) library support"
  WITH_DIRECTX=OFF        # "Include DirectX support"
  WITH_EIGEN=OFF          # "Include Eigen2/Eigen3 support"
  WITH_FFMPEG=OFF         # "Include FFMPEG support"
  WITH_GDAL=OFF           # "Include GDAL Support"
  WITH_GIGEAPI=OFF        # "Include Smartek GigE support"
  WITH_GPHOTO2=OFF        # "Include gPhoto2 library support"
  WITH_GSTREAMER=OFF      # "Include Gstreamer support"
  WITH_GSTREAMER_0_10=OFF # "Enable Gstreamer 0.10 support (instead of 1.x)"
  WITH_GTK=OFF            # "Include GTK support"
  WITH_GTK_2_X=OFF        # "Use GTK version 2"
  WITH_INTELPERC=OFF      # "Include Intel Perceptual Computing support"
  WITH_IPP=OFF            # "Include Intel IPP support"
  WITH_IPP_A=OFF          # "Include Intel IPP_A support"
  WITH_MSMF=OFF           # "Build VideoIO with Media Foundation support"
  WITH_NVCUVID=OFF        # "Include NVidia Video Decoding library support"
  WITH_OPENCL=OFF         # "Include OpenCL Runtime support"
  WITH_OPENCLAMDBLAS=OFF  # "Include AMD OpenCL BLAS library support"
  WITH_OPENCLAMDFFT=OFF   # "Include AMD OpenCL FFT library support"
  WITH_OPENCL_SVM=OFF     # "Include OpenCL Shared Virtual Memory support"
  WITH_OPENEXR=OFF        # "Include ILM support via OpenEXR"
  WITH_OPENGL=OFF         # "Include OpenGL support"
  WITH_OPENMP=OFF         # "Include OpenMP support"
  WITH_OPENNI2=OFF        # "Include OpenNI2 support"
  WITH_OPENNI=OFF         # "Include OpenNI support"
  WITH_PNG=ON             # "Include PNG support"
  WITH_PVAPI=OFF          # "Include Prosilica GigE support"
  WITH_QT=OFF             # "Build with Qt Backend support"
  WITH_QTKIT=NO
  WITH_QUICKTIME=OFF      # "Use QuickTime for Video I/O insted of QTKit"
  WITH_TIFF=OFF           # "Include TIFF support"
  WITH_UNICAP=OFF         # "Include Unicap support (GPL)"
  WITH_VTK=OFF            # "Include VTK library support (and build opencv_viz module eiher)"
  WITH_WEBP=OFF           # "Include WebP support"
  WITH_WIN32UI=OFF        # "Build with Win32 UI Backend support"
  WITH_XIMEA=OFF          # "Include XIMEA cameras support"
  WITH_XINE=OFF           # "Include Xine support (GPL)"
)

if((APPLE OR is_linux) AND NOT IOS)
  list(APPEND OPENCV_CMAKE_ARGS BUILD_JPEG=OFF)
endif()

# "Include Video for Windows support"
if(MSVC)
  list(APPEND OPENCV_CMAKE_ARGS WITH_VFW=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_VFW=OFF)
endif()

# "Include Video 4 Linux support"
if(is_linux)
  list(APPEND OPENCV_CMAKE_ARGS WITH_V4L=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_V4L=OFF)
endif()

# "Include Intel TBB support"
if(is_linux OR MSVC)
  list(APPEND OPENCV_CMAKE_ARGS WITH_TBB=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_TBB=OFF)
endif()

# "Use pthreads-based parallel_for"
if(ANDROID)
  list(APPEND OPENCV_CMAKE_ARGS WITH_PTHREADS_PF=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_PTHREADS_PF=OFF)
endif()

# "Use libv4l for Video 4 Linux support"
if(is_linux)
  list(APPEND OPENCV_CMAKE_ARGS WITH_LIBV4L=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_LIBV4L=OFF)
endif()

# "Include JPEG support"
if(ANDROID OR IOS OR MSVC)
  list(APPEND OPENCV_CMAKE_ARGS WITH_JPEG=OFF)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_JPEG=ON)
endif()

# "Include JPEG2K support"
if(ANDROID OR IOS)
  list(APPEND OPENCV_CMAKE_ARGS WITH_JASPER=OFF)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_JASPER=ON)
endif()

if(ANDROID OR IOS)
  list(APPEND OPENCV_CMAKE_ARGS ENABLE_NEON=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS ENABLE_NEON=OFF)
endif()

# "Use AVFoundation for Video I/O"
if(APPLE AND NOT IOS)
  list(APPEND OPENCV_CMAKE_ARGS WITH_AVFOUNDATION=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_AVFOUNDATION=OFF)
endif()

# "Build VideoIO with DirectShow support"
if(MSVC)
  list(APPEND OPENCV_CMAKE_ARGS WITH_DSHOW=ON)
else()
  list(APPEND OPENCV_CMAKE_ARGS WITH_DSHOW=OFF)
endif()

set(dlib_cmake_args
  DLIB_HEADER_ONLY=OFF  #all previous builds were header on, so that is the default
  DLIB_ENABLE_ASSERTS=OFF #must be set on/off or debug/release build will differ and config will not match one
  DLIB_NO_GUI_SUPPORT=ON
  DLIB_ISO_CPP_ONLY=OFF # needed for directory navigation code (loading training data)
  DLIB_JPEG_SUPPORT=OFF  # https://github.com/hunter-packages/dlib/blob/eb79843227d0be45e1efa68ef9cc6cc187338c8e/dlib/CMakeLists.txt#L422-L432
  DLIB_LINK_WITH_SQLITE3=OFF
  DLIB_USE_BLAS=OFF
  DLIB_USE_LAPACK=OFF
  DLIB_USE_CUDA=OFF
  DLIB_PNG_SUPPORT=ON
  DLIB_GIF_SUPPORT=OFF
  DLIB_USE_MKL_FFT=OFF
  HUNTER_INSTALL_LICENSE_FILES=dlib/LICENSE.txt
  )

# Maintain hunter default args (no testing, license name) and eliminate
# eigen fortrn dependencies
set(EIGEN_CMAKE_ARGS
  BUILD_TESTING=OFF
  HUNTER_INSTALL_LICENSE_FILES=COPYING.MPL2
  CMAKE_Fortran_COMPILER=OFF
)

### Set xgboost args ###
set(XGBOOST_CMAKE_ARGS
  XGBOOST_USE_CEREAL=ON
  XGBOOST_USE_HALF=ON
)

set(acf_cmake_args
  ACF_BUILD_OGLES_GPGPU=ON
  ACF_BUILD_TESTS=OFF
  ACF_BUILD_EXAMPLES=OFF
  ACF_SERIALIZE_WITH_CVMATIO=${DRISHTI_SERIALIZE_WITH_CVMATIO}
  ACF_SERIALIZE_WITH_CEREAL=ON
  ACF_BUILD_OGLES_GPGPU=${DRISHTI_BUILD_OGLES_GPGPU}
)

if(DRISHTI_BUILD_MIN_SIZE)
  list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=ON)
else()
  list(APPEND XGBOOST_CMAKE_ARGS XGBOOST_DO_LEAN=OFF)
endif()

set(OGLES_GPGPU_CMAKE_ARGS
  OGLES_GPGPU_VERBOSE=OFF
  OGLES_GPGPU_OPENGL_ES3=${use_opengl_es3}
)

set(AGLET_CMAKE_ARGS AGLET_OPENGL_ES3=${use_opengl_es3})

hunter_config(ARM_NEON_2_x86_SSE VERSION 1.0.0-p0)
hunter_config(Eigen VERSION 3.3.1-p4 CMAKE_ARGS ${EIGEN_CMAKE_ARGS})
hunter_config(GTest VERSION 1.8.0-hunter-p11)
hunter_config(Jpeg VERSION 9b-p3)
hunter_config(OpenCV VERSION 3.4.1-p1 CMAKE_ARGS "${OPENCV_CMAKE_ARGS}")
hunter_config(PNG VERSION 1.6.26-p1)
hunter_config(RapidXML VERSION 1.13)
hunter_config(aglet VERSION ${HUNTER_aglet_VERSION} CMAKE_ARGS ${AGLET_CMAKE_ARGS}) # test only, use latest
hunter_config(cereal VERSION 1.2.2-p0)
hunter_config(cvmatio VERSION 1.0.28)
hunter_config(dlib VERSION ${HUNTER_dlib_VERSION} CMAKE_ARGS ${dlib_cmake_args})

option(DRISHTI_DRISHTI_AS_SELF "Use drishti as GIT_SELF" OFF)
option(DRISHTI_DRISHTI_AS_SUBMODULE "Use drishti as submodule" OFF)

set(
    drishti_cmake_args
    DRISHTI_OPENGL_ES3=${use_opengl_es3}
    DRISHTI_BUILD_SHARED_SDK=${DRISHTI_BUILD_SHARED_SDK}
)

if(DRISHTI_DRISHTI_AS_SUBMODULE)
  if(NOT DRISHTI_UPLOAD_IGNORE_SUBMODULES)
    hunter_config(drishti GIT_SUBMODULE "src/3rdparty/drishti" CMAKE_ARGS ${drishti_cmake_args})
  endif()
elseif(DRISHTI_DRISHTI_AS_SELF)
  hunter_config(drishti GIT_SELF CMAKE_ARGS ${drishti_cmake_args})
else()
  set(drishti_url "https://github.com/elucideye/drishti/archive/v0.12.4.tar.gz")
  set(drishti_sha1 "02af741ea2d3d4456ba79d9dac8bb2ff46a35e4a")
  hunter_config(drishti URL ${drishti_url} SHA1 ${drishti_sha1} CMAKE_ARGS ${drishti_cmake_args})
  # drishti from Hunter
  # hunter_config(drishti VERSION ${HUNTER_acf_VERSION} CMAKE_ARGS ${acf_cmake_args})
endif()

hunter_config(drishti_assets VERSION 1.8)
hunter_config(drishti_faces VERSION 1.2)
hunter_config(eigen3-nnls VERSION 1.0.1) # eos
hunter_config(eos VERSION 0.12.1) # eos
hunter_config(glfw VERSION 3.3.0-p4)
hunter_config(glm VERSION 0.9.8.5) # eos
hunter_config(half VERSION 1.1.0-p1)
hunter_config(nanoflann VERSION 1.2.3-p0) # eos

set(ogles_gpgpu_url "https://github.com/hunter-packages/ogles_gpgpu/archive/v0.3.1.tar.gz")
set(ogles_gpgpu_sha1 "62f31be45711fa4d4789eb5ff1621d5386eb3433")
hunter_config(ogles_gpgpu URL ${ogles_gpgpu_url} SHA1 ${ogles_gpgpu_sha1} CMAKE_ARGS ${OGLES_GPGPU_CMAKE_ARGS})
#hunter_config(ogles_gpgpu VERSION ${HUNTER_ogles_gpgpu_VERSION} CMAKE_ARGS ${OGLES_GPGPU_CMAKE_ARGS})

hunter_config(spdlog VERSION 0.13.0-p0)
hunter_config(sse2neon VERSION 1.0.0-p0)
hunter_config(thread-pool-cpp VERSION 1.1.0)
hunter_config(xgboost VERSION 0.40-p10 CMAKE_ARGS ${XGBOOST_CMAKE_ARGS})

option(DRISHTI_ACF_AS_SUBMODULE "Use ACF as a submodule" OFF)
option(DRISHTI_ACF_AS_SELF "Use ACF as a GI_SELF" OFF)

if(DRISHTI_ACF_AS_SUBMODULE)
  if(NOT DRISHTI_UPLOAD_IGNORE_SUBMODULES)
    hunter_config(acf GIT_SUBMODULE "src/3rdparty/acf" CMAKE_ARGS ${acf_cmake_args})
  endif()
elseif(DRISHTI_ACF_AS_SELF)
  hunter_config(acf GIT_SELF CMAKE_ARGS ${acf_cmake_args})
else()
  # ACF from direct URL:
  set(acf_url "https://github.com/elucideye/acf/archive/v0.1.16.tar.gz")
  set(acf_sha1 "d1f681d6e678fcc45777e780f40fc196aad4c726")
  hunter_config(acf URL ${acf_url} SHA1 ${acf_sha1} CMAKE_ARGS ${acf_cmake_args})
  # ACF from Hunter
  # hunter_config(acf VERSION ${HUNTER_acf_VERSION} CMAKE_ARGS ${acf_cmake_args})
endif()

# experimental: lock verison but not used for CI builds
hunter_config(dest VERSION 0.8.0-p4)
hunter_config(flatbuffers VERSION 1.8.0-p1)
hunter_config(tinydir VERSION 1.2-p0)
hunter_config(Beast VERSION 1.0.0-b32-hunter-4)

if(NOT DRISHTI_UPLOAD_IGNORE_SUBMODULES)

  # Note: MSVC currently broken due to internal GL_BGR(A) enums
  # TODO: Update imshow package
  if(NOT (ANDROID OR IOS OR MSVC))
    hunter_config(imshow GIT_SUBMODULE "src/3rdparty/imshow")
  endif()
endif()

# Requirements for Urho3D {

# * https://docs.hunter.sh/en/latest/packages/pkg/Urho3D.html
hunter_config(Lua VERSION 5.1.5-p3)
hunter_config(SDL2 VERSION 2.0.4-urho-p4)

# }
