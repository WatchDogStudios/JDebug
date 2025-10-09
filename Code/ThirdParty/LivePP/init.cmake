# LIVE++ Utils
# NOTES: To not use github storage, you can download the Live++ SDK from the official website and put it in the same directory as this file.

if(NS_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    set(NS_3RDPARTY_LIVEPP_SUPPORT OFF CACHE BOOL "Enable Live++ support for Ns")
endif()

# #####################################
# ## ns_requires_livepp()
# #####################################
macro(ns_requires_livepp)
    ns_requires(NS_3RDPARTY_LIVEPP_SUPPORT)
endmacro()

function(ns_export_directory TARGET_NAME)
    ns_requires_livepp()
    # NOTE: Before we export anything, we need to verify that the Live++ SDK is present.
    if(NOT EXISTS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/Broker/LPP_Broker.exe)
        message(FATAL_ERROR "Live++ SDK (Broker or Agent) not found. Please download the SDK from the official website (https://liveplusplus.tech/) and put it in the same directory as this file.")
    endif()
    add_custom_command(TARGET ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_FUNCTION_LIST_DIR} $<TARGET_FILE_DIR:${TARGET_NAME}>/LivePP
        WORKING_DIRECTORY ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
    )
endfunction()

function(ns_target_link_livepp TARGET_NAME)
    ns_requires_livepp()
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/API/x64)
    target_compile_definitions(${TARGET_NAME} PRIVATE LIVEPP_ENABLED=1)
    ns_export_directory(${TARGET_NAME})
endfunction()
