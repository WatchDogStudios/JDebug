ns_cmake_init()

# Stack resolver is implemented using DbgHelp.h and thus only supports windows right now.
ns_requires(NS_CMAKE_PLATFORM_WINDOWS_DESKTOP)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ns_create_target(APPLICATION ${PROJECT_NAME})

ns_add_output_ns_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  Foundation
  dbghelp
)
