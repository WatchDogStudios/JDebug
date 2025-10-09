ns_cmake_init()

ns_requires_vulkan()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ns_create_target(LIBRARY ${PROJECT_NAME} NO_NS_PREFIX)

ns_link_target_vulkan(${PROJECT_NAME})

# Public include path so other modules can consume headers

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(${PROJECT_NAME}
  PRIVATE
    VulkanRendererPCH.cpp
    DxcSupport.cpp
    VulkanRendererModule.cpp
    Core/VkInstance.cpp
    Core/VkDevice.cpp
    Core/VkSwapChain.cpp
    Core/VkCommandContext.cpp
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Foundation
    Core
  PRIVATE
    volk
)
