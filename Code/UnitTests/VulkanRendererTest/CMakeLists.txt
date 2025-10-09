ns_cmake_init()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ns_create_target(APPLICATION ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    VulkanRendererTestPCH.cpp
    VulkanRendererTest.cpp
    Basics/VulkanShaderSmokeTest.cpp
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    TestFramework
    Foundation
)

ns_project_build_filter_index(${PROJECT_NAME} BUILD_FILTER_IDX)

ns_ci_add_test(${PROJECT_NAME})
