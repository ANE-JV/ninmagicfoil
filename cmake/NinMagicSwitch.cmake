# Use a project-local, pinned libnx without modifying the developer's installed SDK.
include("$ENV{DEVKITPRO}/cmake/Switch.cmake")
if(NOT DEFINED ENV{NINMAGIC_LIBNX_ROOT})
    message(FATAL_ERROR "Run tools/build-ninmagicfoil.ps1 or tools/build-ninmagicfoil.sh to prepare the pinned libnx")
endif()
# Keep the installed prefix as a fallback for deko3d, installed alongside libnx.
# The explicit platform include/link flags and first search prefix use local libnx.
set(NX_ROOT "$ENV{NINMAGIC_LIBNX_ROOT}")
if(NOT EXISTS "${NX_ROOT}/lib/libnx.a")
    message(FATAL_ERROR "Pinned libnx was not built: ${NX_ROOT}")
endif()
__dkp_platform_prefix("${NX_ROOT}")
