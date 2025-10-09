ns_cmake_init()

ns_requires_desktop()
ns_requires_qt()

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE NO_NS_PREFIX)

ns_create_target(APPLICATION ${PROJECT_NAME})

ns_add_output_ns_prefix(${PROJECT_NAME})

set(NS_MOC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.h
  ${CMAKE_CURRENT_SOURCE_DIR}/JDebugViewportWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/LogDockWidget.h
)

ns_qt_wrap_target_moc_files(${PROJECT_NAME} "${NS_MOC_HEADERS}")

ns_link_target_qt(TARGET ${PROJECT_NAME} COMPONENTS Core Gui Widgets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    JVDSDK
    Foundation
    Core
  VulkanRenderer
    PvdRenderer
)
