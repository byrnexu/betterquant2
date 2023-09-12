include(ExternalProject)
include(cmake/config.cmake)

set(ROBIN_MAP_MAJOR_VER 1)
set(ROBIN_MAP_MINOR_VER 2)
set(ROBIN_MAP_PATCH_VER 1)
set(ROBIN_MAP_URL_HASH  SHA256=2b54d2c1de2f73bea5c51d5dcbd64813a08caf1bfddcfdeee40ab74e9599e8e3)

set(ROBIN_MAP_VER     ${ROBIN_MAP_MAJOR_VER}.${ROBIN_MAP_MINOR_VER}.${ROBIN_MAP_PATCH_VER})
set(ROBIN_MAP_ROOT    ${3RDPARTY_PATH}/robin-map)
set(ROBIN_MAP_INC_DIR ${ROBIN_MAP_ROOT}/src/robin-map-${ROBIN_MAP_VER}/include)
set(ROBIN_MAP_INSTALL echo "install robin-map to ${ROBIN_MAP_INC_DIR}")

set(ROBIN_MAP_URL https://github.com/Tessil/robin-map/archive/refs/tags/v${ROBIN_MAP_VER}.tar.gz)

ExternalProject_Add(robin-map-${ROBIN_MAP_VER}
    URL               ${ROBIN_MAP_URL}
    URL_HASH          ${ROBIN_MAP_URL_HASH} 
    DOWNLOAD_NAME     robin-map-${ROBIN_MAP_VER}.tar.gz
    PREFIX            ${ROBIN_MAP_ROOT}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ${ROBIN_MAP_INSTALL}
    )

set(3RDPARTY_DEPENDENCIES ${3RDPARTY_DEPENDENCIES} robin-map-${ROBIN_MAP_VER})

if (NOT EXISTS ${ROBIN_MAP_ROOT}/src/robin-map-${ROBIN_MAP_VER})
    add_custom_target(rescan-robin-map ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} DEPENDS robin-map-${ROBIN_MAP_VER})
else()
    add_custom_target(rescan-robin-map)
endif()

