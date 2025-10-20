ns_cmake_init()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

ns_create_target(LIBRARY ${PROJECT_NAME} NO_NS_PREFIX)

# Public include path so other modules can consume headers
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(${PROJECT_NAME}
  PRIVATE
    DirectX11RendererPCH.cpp
    DirectX11RendererModule.cpp
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Foundation
    Core
)

# Link DirectX11 libraries on Windows
if(NS_CMAKE_PLATFORM_WINDOWS)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      d3d11.lib
      dxgi.lib
      d3dcompiler.lib
  )
endif()
