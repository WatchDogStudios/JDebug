######################################
### Jolt support
######################################

set (NS_3RDPARTY_KRAUT_SUPPORT ON CACHE BOOL "Whether to add support for procedurally generated trees with Kraut.")
mark_as_advanced(FORCE NS_3RDPARTY_KRAUT_SUPPORT)

######################################
### ns_requires_kraut()
######################################

macro(ns_requires_kraut)

	ns_requires(NS_3RDPARTY_KRAUT_SUPPORT)

endmacro()
