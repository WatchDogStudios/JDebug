ns_cmake_init()

ns_requires_qt()
ns_requires(NS_CMAKE_PLATFORM_WINDOWS)

ns_requires(NS_3RDPARTY_ENET_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ns_create_target(APPLICATION ${PROJECT_NAME})

ns_make_windowapp(${PROJECT_NAME})

ns_add_output_ns_prefix(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Foundation
  FileservePlugin
)

ns_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets Network)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  EditorPluginFileserve
  RendererCore
)

