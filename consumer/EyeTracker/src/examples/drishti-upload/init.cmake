set(
    HUNTER_CACHE_SERVERS
    "https://github.com/elucideye/hunter-cache"
    CACHE
    STRING
    "Hunter cache servers"
)

# https://docs.travis-ci.com/user/environment-variables/#Default-Environment-Variables
string(COMPARE EQUAL "$ENV{TRAVIS}" "true" is_travis)

# https://www.appveyor.com/docs/environment-variables/
string(COMPARE EQUAL "$ENV{APPVEYOR}" "True" is_appveyor)

string(COMPARE EQUAL "$ENV{GITHUB_USER_PASSWORD}" "" password_is_empty)

if(password_is_empty)
  set(default_upload OFF)
elseif(is_travis OR is_appveyor)
  set(default_upload ON)
else()
  set(default_upload OFF)
endif()

option(HUNTER_RUN_UPLOAD "Upload cache binaries" ${default_upload})
message("HUNTER_RUN_UPLOAD: ${HUNTER_RUN_UPLOAD}")

set(
    HUNTER_PASSWORDS_PATH
    "${CMAKE_CURRENT_LIST_DIR}/passwords.cmake"
    CACHE
    FILEPATH
    "Hunter passwords"
)

option(HUNTER_GIT_SELF_IGNORE_UNTRACKED "GIT_SELF: Ingored untracked files" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

if(DRISHTI_HUNTER_CONFIG_MINIMAL)
  set(config_name "minimal")
else()
  set(config_name "config")
endif()

# HunterGate.cmake should be included by parent project. Parent project should
# have this module anyway because it will be used in Hunter without
# 'drishti-upload' submodule.  This call *is* used for the CI builds/tests
# of the drishti repository.
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.14.tar.gz"
    SHA1 "f58bcf274f29097ac380bfe4955d2a66f077e741"
    FILEPATH "${CMAKE_CURRENT_LIST_DIR}/${config_name}.cmake"
)
