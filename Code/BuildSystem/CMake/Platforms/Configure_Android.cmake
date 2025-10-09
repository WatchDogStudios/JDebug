include("${CMAKE_CURRENT_LIST_DIR}/Configure_Default.cmake")

message(STATUS "Configuring Platform: Android")

set_property(GLOBAL PROPERTY NS_CMAKE_PLATFORM_ANDROID ON)
set_property(GLOBAL PROPERTY NS_CMAKE_PLATFORM_POSIX ON)
set_property(GLOBAL PROPERTY NS_CMAKE_PLATFORM_SUPPORTS_VULKAN ON)

macro(ns_platform_pull_properties)

	get_property(NS_CMAKE_PLATFORM_ANDROID GLOBAL PROPERTY NS_CMAKE_PLATFORM_ANDROID)

endmacro()

macro(ns_platform_detect_generator)

	if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
		message(STATUS "Buildsystem is Ninja (NS_CMAKE_GENERATOR_NINJA)")

		set_property(GLOBAL PROPERTY NS_CMAKE_GENERATOR_NINJA ON)
		set_property(GLOBAL PROPERTY NS_CMAKE_GENERATOR_PREFIX "Ninja")
		set_property(GLOBAL PROPERTY NS_CMAKE_GENERATOR_CONFIGURATION ${CMAKE_BUILD_TYPE})

	else()
		message(FATAL_ERROR "Generator '${CMAKE_GENERATOR}' is not supported on Android! Please extend ns_platform_detect_generator()")
	endif()

endmacro()

macro(ns_platformhook_link_target_vulkan TARGET_NAME)

	# on linux is the loader a dll
	get_target_property(_dll_location NsVulkan::Loader IMPORTED_LOCATION)

	if(NOT _dll_location STREQUAL "")
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:NsVulkan::Loader> $<TARGET_FILE_DIR:${TARGET_NAME}>)
	endif()

	unset(_dll_location)

endmacro()

macro(ns_platformhook_set_build_flags_clang TARGET_NAME)
	target_compile_options(${TARGET_NAME} PRIVATE -fPIC)

	# Look for the super fast ld compatible linker called "mold". If present we want to use it.
	find_program(MOLD_PATH "mold")

	# We want to use the llvm linker lld by default
	# Unless the user has specified a different linker
	get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)

	if("${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY")
		if(NOT("${CMAKE_EXE_LINKER_FLAGS}" MATCHES "fuse-ld="))
			# TODO_ANDROID: Mold does not support `--undefined-glob` so we can't use it for Android right now. Either we need to prevent via some other means that the linger drops plugins or figure out which version of mold if any supports `--undefined-glob`.
			if(false) #if(MOLD_PATH)
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=${MOLD_PATH}")
			else()
				target_link_options(${TARGET_NAME} PRIVATE "-fuse-ld=lld")
			endif()
		endif()

		# Reporting missing symbols at linktime
		target_link_options(${TARGET_NAME} PRIVATE "-Wl,-z,defs")
		# Prevent discarding of statically linked plugins
		target_link_options(${TARGET_NAME} PRIVATE "LINKER:--undefined-glob=*nsReferenceFunction*")		
	endif()
endmacro()

macro(ns_platformhook_find_vulkan)

	# As we are cross compiling, CMake assumes every path to be located under the Android NDK root. This is not the case for external libraries like the Vulkan SDK, so we need to clear the sysroot and find root path.
	set(backup_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
	set(backup_CMAKE_SYSROOT ${CMAKE_SYSROOT})
	set(CMAKE_FIND_ROOT_PATH "")
	set(CMAKE_SYSROOT "")

	if(NS_CMAKE_ARCHITECTURE_64BIT)
		if((NS_VULKAN_DIR STREQUAL "NS_VULKAN_DIR-NOTFOUND") OR(NS_VULKAN_DIR STREQUAL ""))
			#set(CMAKE_FIND_DEBUG_MODE TRUE)
			unset(NS_VULKAN_DIR CACHE)
			unset(NsVulkan_DIR CACHE)
			set(NS_SHARED_VULKAN_DIR "${NS_ROOT}/Workspace/shared/vulkan-sdk/${NS_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
			find_path(NS_VULKAN_DIR config/vk_layer_settings.txt
					PATHS
					${NS_SHARED_VULKAN_DIR}
					${NS_VULKAN_DIR}
					$ENV{VULKAN_SDK}
			)
			#set(CMAKE_FIND_DEBUG_MODE FALSE)

			if((NS_VULKAN_DIR STREQUAL "NS_VULKAN_DIR-NOTFOUND") OR (NS_VULKAN_DIR STREQUAL ""))
				# When cross-compiling on windows, we assume the env var VULKAN_SDK is set so the previous find_path call should have succeeded.
				# On Linux, we just download the SDK as we would do when building for Linux directly.
				# This is a bit wasteful as we already downloaded it and we only need a few headers, but cross workspace dependencies aren't easy to define in cmake. 
				if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
					# To prevent race-conditions if two CMake presets are updated at the same time, we download into the local workspace and then create a link into the shared directory.
					ns_download_and_extract("${NS_CONFIG_VULKAN_SDK_LINUXX64_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-sdk-${NS_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
					ns_create_link("${CMAKE_BINARY_DIR}/vulkan-sdk/${NS_CONFIG_VULKAN_SDK_LINUXX64_VERSION}" "${NS_ROOT}/Workspace/shared/vulkan-sdk/" "${NS_CONFIG_VULKAN_SDK_LINUXX64_VERSION}")
					set(NS_VULKAN_DIR "${NS_SHARED_VULKAN_DIR}" CACHE PATH "Directory of the Vulkan SDK" FORCE)

					find_path(NS_VULKAN_DIR config/vk_layer_settings.txt NO_DEFAULT_PATH
							PATHS
							${NS_VULKAN_DIR}
							$ENV{VULKAN_SDK}
					)
				endif ()
			endif()

			if((NS_VULKAN_DIR STREQUAL "NS_VULKAN_DIR-NOTFOUND") OR (NS_VULKAN_DIR STREQUAL ""))
				message(FATAL_ERROR "Failed to find vulkan SDK. Ns requires the vulkan sdk ${NS_CONFIG_VULKAN_SDK_LINUXX64_VERSION}. Please set the environment variable VULKAN_SDK to the vulkan sdk location.")
			endif()
		endif()
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()

	set(NS_VULKAN_VALIDATIONLAYERS_DIR "" CACHE PATH "Directory of the Vulkan Validation Layers")

	# Download prebuilt VkLayer_khronos_validation for Android
	if((NS_VULKAN_VALIDATIONLAYERS_DIR STREQUAL "NS_VULKAN_VALIDATIONLAYERS_DIR-NOTFOUND") OR (NS_VULKAN_VALIDATIONLAYERS_DIR STREQUAL ""))
		
		set(NS_SHARED_VULKAN_VALIDATIONLAYERS_DIR "${NS_ROOT}/Workspace/shared/vulkan-sdk/android-binaries-${NS_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
		
		#set(CMAKE_FIND_DEBUG_MODE TRUE)
		unset(NS_VULKAN_VALIDATIONLAYERS_DIR CACHE)
		find_path(NS_VULKAN_VALIDATIONLAYERS_DIR
				NAMES arm64-v8a/libVkLayer_khronos_validation.so
				NO_DEFAULT_PATH
				HINTS
				${NS_SHARED_VULKAN_VALIDATIONLAYERS_DIR}
				${NS_VULKAN_VALIDATIONLAYERS_DIR}
		)
		#set(CMAKE_FIND_DEBUG_MODE FALSE)
		if((NS_VULKAN_VALIDATIONLAYERS_DIR STREQUAL "NS_VULKAN_VALIDATIONLAYERS_DIR-NOTFOUND") OR (NS_VULKAN_VALIDATIONLAYERS_DIR STREQUAL ""))
			ns_download_and_extract("${NS_CONFIG_VULKAN_VALIDATIONLAYERS_ANDROID_URL}" "${CMAKE_BINARY_DIR}/vulkan-sdk" "vulkan-layers-${NS_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
			ns_create_link("${CMAKE_BINARY_DIR}/vulkan-sdk/android-binaries-${NS_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}" "${NS_ROOT}/Workspace/shared/vulkan-sdk/" "android-binaries-${NS_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}")
			set(NS_VULKAN_VALIDATIONLAYERS_DIR "${NS_ROOT}/Workspace/shared/vulkan-sdk/android-binaries-${NS_CONFIG_VULKAN_VALIDATIONLAYERS_VERSION}" CACHE PATH "Directory of the Vulkan Validation Layers" FORCE)

			find_path(NS_VULKAN_VALIDATIONLAYERS_DIR arm64-v8a/libVkLayer_khronos_validation.so NO_DEFAULT_PATH
				PATHS
				${NS_SHARED_VULKAN_VALIDATIONLAYERS_DIR}
				${NS_VULKAN_VALIDATIONLAYERS_DIR}
			)
		endif()
	endif()

	set(CMAKE_FIND_ROOT_PATH ${backup_CMAKE_FIND_ROOT_PATH})
	set(CMAKE_SYSROOT ${backup_CMAKE_SYSROOT})

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(NsVulkan DEFAULT_MSG NS_VULKAN_DIR)

	if(NOT ANDROID_NDK)
		message(WARNING "ANDROID_NDK not set")

		if(NOT EXISTS "$ENV{ANDROID_NDK_HOME}")
			message(FATAL_ERROR "ANDROID_NDK_HOME environment variable not set. Please ensure it points to the android NDK root folder.")
		else()
			set(ANDROID_NDK $ENV{ANDROID_NDK_HOME})
		endif()
	endif()

	if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
		set(NS_VULKAN_INCLUDE_DIR "${NS_VULKAN_DIR}/x86_64/include")
	elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
		set(NS_VULKAN_INCLUDE_DIR "${NS_VULKAN_DIR}/Include")
	else ()
		message(FATAL_ERROR "Unknown host system, can't find vulkan include dir.")
	endif ()

	if(NS_CMAKE_ARCHITECTURE_64BIT)
		if(NS_CMAKE_ARCHITECTURE_ARM)
			add_library(NsVulkan::Loader SHARED IMPORTED)
			set_target_properties(NsVulkan::Loader PROPERTIES IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/${ANDROID_PLATFORM}/libvulkan.so")
			set_target_properties(NsVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NS_VULKAN_INCLUDE_DIR}")
		elseif(NS_CMAKE_ARCHITECTURE_X86)
			add_library(NsVulkan::Loader SHARED IMPORTED)
			set_target_properties(NsVulkan::Loader PROPERTIES IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/x86_64-linux-android/${ANDROID_PLATFORM}/libvulkan.so")
			set_target_properties(NsVulkan::Loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NS_VULKAN_INCLUDE_DIR}")
		endif()
		# We define NsVulkan::DXC as a stub as we can't compile on android, but the high level init cmake code expects this function to define it.
		add_library(NsVulkan::DXC SHARED IMPORTED)
	else()
		message(FATAL_ERROR "TODO: Vulkan is not yet supported on this platform and/or architecture.")
	endif()

endmacro()

macro(ns_platformhook_package_files TARGET_NAME SRC_FOLDER DST_FOLDER)

	# Package files for Android APK by copying them to the Assets directory
	# This is done in PRE_BUILD so that it happens before the APK generation steps
	# that happen in POST_BUILD via ns_create_target
	set(ANDROID_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/package/Assets")
	
	add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ANDROID_PACKAGE_DIR}/${DST_FOLDER}"
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${SRC_FOLDER}" "${ANDROID_PACKAGE_DIR}/${DST_FOLDER}"
		COMMENT "Packaging ${SRC_FOLDER} -> ${DST_FOLDER} for Android")

endmacro()