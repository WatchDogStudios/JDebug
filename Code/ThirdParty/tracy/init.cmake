if(NS_CMAKE_PLATFORM_WINDOWS_DESKTOP)
    set (NS_3RDPARTY_TRACY_SUPPORT ON CACHE BOOL "Whether to add support for profiling the engine with Tracy.")
	set (NS_3RDPARTY_TRACY_TRACK_ALLOCATIONS OFF CACHE BOOL "Whether Tracy should track memory allocations. Warning: disables C++ code reload!")
else()
    # Tracy currently doesn't compile on Linux
    set (NS_3RDPARTY_TRACY_SUPPORT OFF CACHE BOOL "Whether to add support for profiling the engine with Tracy.")
endif()

if (NOT NS_3RDPARTY_TRACY_SUPPORT)
	unset(NS_3RDPARTY_TRACY_TRACK_ALLOCATIONS CACHE)
endif()