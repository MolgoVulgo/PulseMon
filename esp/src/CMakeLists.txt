set(APP_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
file(GLOB_RECURSE app_sources ${APP_SRC_DIR}/*.*)

set(app_include_dirs ${APP_SRC_DIR})
if(EXISTS "${APP_SRC_DIR}/ui")
    list(APPEND app_include_dirs "${APP_SRC_DIR}/ui")
endif()

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS ${app_include_dirs}
)
