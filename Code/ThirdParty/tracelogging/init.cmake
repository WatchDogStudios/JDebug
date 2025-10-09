ns_requires(NS_CMAKE_PLATFORM_LINUX)

set (NS_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT OFF CACHE BOOL "Whether to add support for tracelogging via lttng.")
mark_as_advanced(FORCE NS_3RDPARTY_TRACELOGGING_LTTNG_SUPPORT)