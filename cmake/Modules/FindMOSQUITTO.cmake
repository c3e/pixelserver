find_path(MOSQUITTO_INCLUDE_DIRS NAMES mosquitto.h
  PATH_SUFFIXES mosquitto
  DOC "The mosquitto include directory")

find_library(MOSQUITTO_LIBRARIES NAMES mosquitto
  DOC "The mosquitto libarary")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MOSQUITTO REQUIRED_VARS MOSQUITTO_LIBRARIES MOSQUITTO_INCLUDE_DIRS)

if(MOSQUITTO_FOUND)
  set(MOSQUITTO_LIBRARY ${MOSQUITTO_LIBRARIES})
  set(MOSQUITTO_INCLUDE_DIR ${MOSQUITTO_INCLUDE_DIRS})
endif()

