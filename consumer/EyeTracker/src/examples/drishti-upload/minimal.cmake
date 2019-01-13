message("Using config 'minimal'")

hunter_config(
    xgboost
    VERSION 0.40-p10
    CMAKE_ARGS
    XGBOOST_USE_HALF=ON
    XGBOOST_USE_CEREAL=ON
    XGBOOST_DO_LEAN=ON
)

hunter_config(
    dlib
    VERSION
    ${HUNTER_dlib_VERSION}
    CMAKE_ARGS
    DLIB_USE_BLAS=OFF
)

set(acf_url "https://github.com/elucideye/acf/archive/v0.1.16.tar.gz")
set(acf_sha1 "d1f681d6e678fcc45777e780f40fc196aad4c726")
hunter_config(acf URL ${acf_url} SHA1 ${acf_sha1})
