ns_cmake_init()

ns_requires_vulkan()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

set(PVD_RENDERER_SOURCES
  PvdRendererPCH.cpp
  Renderer/PvdVulkanRenderer.cpp
)

message(STATUS "PVD_RENDERER_SOURCES=${PVD_RENDERER_SOURCES}")

ns_create_target(LIBRARY ${PROJECT_NAME} NO_PCH NO_UNITY MANUAL_SOURCE_FILES ${PVD_RENDERER_SOURCES})

set_source_files_properties(
  Components/Implementation/PvdBodyComponent.cpp
  PvdRendererModule.cpp
  Renderer/PvdBodyPass.cpp
  Renderer/PvdBodyRenderer.cpp
  Renderer/PvdRenderData.cpp
  Renderer/PvdRendererPipelines.cpp
  Scene/PvdDefaultSceneBundle.cpp
  PROPERTIES HEADER_FILE_ONLY TRUE
)

ns_link_target_vulkan(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Foundation
    JVDSDK
    VulkanRenderer
)
