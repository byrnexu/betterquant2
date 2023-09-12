include(ExternalProject)
include(cmake/config.cmake)

set(UNORDERED_DENSE_MAJOR_VER 3)
set(UNORDERED_DENSE_MINOR_VER 0)
set(UNORDERED_DENSE_PATCH_VER 1)
set(UNORDERED_DENSE_URL_HASH  SHA256=37085a787930adf36da89185a80236d3f6a29970017e88d88f731acf42e68d6b)

set(UNORDERED_DENSE_VER     ${UNORDERED_DENSE_MAJOR_VER}.${UNORDERED_DENSE_MINOR_VER}.${UNORDERED_DENSE_PATCH_VER})
set(UNORDERED_DENSE_ROOT    ${3RDPARTY_PATH}/unordered-dense)
set(UNORDERED_DENSE_INC_DIR ${UNORDERED_DENSE_ROOT}/src/unordered-dense-${UNORDERED_DENSE_VER}/include)
set(UNORDERED_DENSE_INSTALL echo "install unordered-dense to ${UNORDERED_DENSE_INC_DIR}")

set(UNORDERED_DENSE_URL https://github.com/martinus/unordered_dense/archive/refs/tags/v${UNORDERED_DENSE_VER}.tar.gz)

ExternalProject_Add(unordered-dense-${UNORDERED_DENSE_VER}
    URL               ${UNORDERED_DENSE_URL}
    URL_HASH          ${UNORDERED_DENSE_URL_HASH} 
    DOWNLOAD_NAME     unordered-dense-${UNORDERED_DENSE_VER}.tar.gz
    PREFIX            ${UNORDERED_DENSE_ROOT}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ${UNORDERED_DENSE_INSTALL}
    )

set(3RDPARTY_DEPENDENCIES ${3RDPARTY_DEPENDENCIES} unordered-dense-${UNORDERED_DENSE_VER})

if (NOT EXISTS ${UNORDERED_DENSE_ROOT}/src/unordered-dense-${UNORDERED_DENSE_VER})
    add_custom_target(rescan-unordered-dense ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} DEPENDS unordered-dense-${UNORDERED_DENSE_VER})
else()
    add_custom_target(rescan-unordered-dense)
endif()

