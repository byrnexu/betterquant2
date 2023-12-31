cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

file(GLOB 3RDPARTY_LIST cmake/*.cmake)
foreach(3RDPARTY_LIB ${3RDPARTY_LIST})
    include (${3RDPARTY_LIB})
endforeach()

project(bqstgeng C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CXX_FLAGS
    -g
    -Wextra
    -Werror
    -Wno-unused-parameter
    -march=native
    )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fPIC")

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

set(EXECUTABLE_OUTPUT_PATH ${SOLUTION_ROOT_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${SOLUTION_ROOT_DIR}/lib)

check_if_the_cmd_exists(clang-format)
get_proj_ver(${PROJ_VER} ${MAJOR_VER})

configure_file (
    "${PROJECT_SOURCE_DIR}/config.hpp.in"
    "${PROJECT_BINARY_DIR}/config-proj.hpp")

aux_source_directory(src SRC_LIST)
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_link_options(${PROJECT_NAME} PUBLIC "LINKER:--copy-dt-needed-entries")

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "-d")
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJ_VER} SOVERSION ${MAJOR_VER})
else()
    message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJ_VER} SOVERSION ${MAJOR_VER})
endif()

add_dependencies(${PROJECT_NAME} ${3RDPARTY_DEPENDENCIES})
message(STATUS "3RDPARTY_DEPENDENCIES ${3RDPARTY_DEPENDENCIES}")

target_include_directories(${PROJECT_NAME}
    PUBLIC "${SOLUTION_ROOT_DIR}/bqalgo/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/bqstg/bqstgengimpl/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/bqposmgr/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/bqordmgr/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/bqweb/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/bqipc/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/bqpub/inc"
    PUBLIC "${SOLUTION_ROOT_DIR}/pub/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/inc"
    PUBLIC "${PROJECT_SOURCE_DIR}/src"
    PUBLIC "${PROJECT_BINARY_DIR}"
    PUBLIC "${TAOS_INC_DIR}"
    PUBLIC "${ABSEIL_INC_DIR}"
    PUBLIC "${ICEORYX_INC_DIR}"
    PUBLIC "${YYJSON_INC_DIR}"
    PUBLIC "${RAPIDJSON_INC_DIR}"
    PUBLIC "${UNORDERED_DENSE_INC_DIR}"
    PUBLIC "${NLOHMANN_JSON_INC_DIR}"
    PUBLIC "${CPR_INC_DIR}"
    PUBLIC "${CURL_INC_DIR}"
    PUBLIC "${YAMLCPP_INC_DIR}"
    PUBLIC "${WEBSOCKETPP_INC_DIR}"
    PUBLIC "${SPDLOG_INC_DIR}"
    PUBLIC "${BOOST_INC_DIR}"
    PUBLIC "${READERWRITER_QUEUE_INC_DIR}"
    PUBLIC "${CONCURRENT_QUEUE_INC_DIR}"
    PUBLIC "${GFLAGS_INC_DIR}"
    PUBLIC "${MAGIC_ENUM_INC_DIR}"
    PUBLIC "${FMT_INC_DIR}"
    PUBLIC "${XXHASH_INC_DIR}"
    )

target_link_directories(${PROJECT_NAME}
    PUBLIC "${SOLUTION_ROOT_DIR}/lib/"
    PUBLIC "${ABSEIL_LIB_DIR}"
    PUBLIC "${ICEORYX_LIB_DIR}"
    PUBLIC "${TAOS_LIB_DIR}"
    PUBLIC "${YYJSON_LIB_DIR}"
    PUBLIC "${NLOHMANN_JSON_LIB_DIR}"
    PUBLIC "${CPR_LIB_DIR}"
    PUBLIC "${CURL_LIB_DIR}"
    PUBLIC "${YAMLCPP_LIB_DIR}"
    PUBLIC "${WEBSOCKETPP_LIB_DIR}"
    PUBLIC "${SPDLOG_LIB_DIR}"
    PUBLIC "${BOOST_LIB_DIR}"
    PUBLIC "${READERWRITER_QUEUE_LIB_DIR}"
    PUBLIC "${GFLAGS_LIB_DIR}"
    PUBLIC "${MAGIC_ENUM_LIB_DIR}"
    PUBLIC "${FMT_LIB_DIR}"
    PUBLIC "${XXHASH_LIB_DIR}"
    )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME}
      PUBLIC bqalgo-d
      PUBLIC bqposmgr-d
      PUBLIC bqordmgr-d
      PUBLIC bqipc-d
      PUBLIC bqpub-d
      PUBLIC pub-d
      )
else()
    target_link_libraries(${PROJECT_NAME}
      PUBLIC bqalgo
      PUBLIC bqposmgr
      PUBLIC bqordmgr
      PUBLIC bqipc
      PUBLIC bqpub
      PUBLIC pub
      )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC taos
    PUBLIC libcpr.a
    PUBLIC libcurl.a
    PUBLIC iceoryx_posh
    PUBLIC iceoryx_hoofs
    PUBLIC iceoryx_platform
    PUBLIC iceoryx_posh_config
    PUBLIC iceoryx_binding_c
    PUBLIC iceoryx_posh_gateway
    PUBLIC iceoryx_posh_roudi
    PUBLIC libyyjson.a
    PUBLIC libxxhash.a
    PUBLIC libabsl_raw_hash_set.a
    PUBLIC libabsl_flags_reflection.a
    PUBLIC libabsl_hash.a
    PUBLIC libabsl_city.a
    PUBLIC libabsl_low_level_hash.a
    PUBLIC libyaml-cpp.a
    PUBLIC libfmt.a
    PUBLIC libgflags.a
    PUBLIC libmimalloc.a
    PUBLIC dl
    PUBLIC pthread
    )

option(BUILD_TESTS "Build the tests" ON)
if (BUILD_TESTS)
    set(TEST_PROJECT_NAME ${PROJECT_NAME}-test)
    message(STATUS "Start building test cases.")
    enable_testing()
    add_test(NAME test COMMAND ${TEST_PROJECT_NAME} WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test)
    add_subdirectory(test)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        add_custom_target(tests COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST_PROJECT_NAME}-d)
    else()
        add_custom_target(tests COMMAND ${EXECUTABLE_OUTPUT_PATH}/${TEST_PROJECT_NAME})
    endif()
endif()

option(BUILD_BENCH "Build the bench" ON)
if (BUILD_BENCH)
    set(BENCH_PROJECT_NAME ${PROJECT_NAME}-bench)
    message(STATUS "Start building benches.")
    add_subdirectory(bench)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        add_custom_target(bench COMMAND ${EXECUTABLE_OUTPUT_PATH}/${BENCH_PROJECT_NAME}-d)
    else()
        add_custom_target(bench COMMAND ${EXECUTABLE_OUTPUT_PATH}/${BENCH_PROJECT_NAME})
    endif()
endif()

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
execute_process(COMMAND bash -c "
ar -M << EOM
    CREATE ${SOLUTION_ROOT_DIR}/lib/libbqstgeng-cxx-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqstgeng-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqalgo-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqstgengimpl-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqposmgr-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqordmgr-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqpub-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqipc-d.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libpub-d.a
    ADDLIB ${CPR_LIB_DIR}/libcpr.a
    ADDLIB ${CURL_LIB_DIR}/libcurl.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_raw_hash_set.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_flags_reflection.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_hash.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_city.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_low_level_hash.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_binding_c.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_hoofs.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_platform.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh_config.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh_gateway.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh_roudi.a
    ADDLIB ${GFLAGS_LIB_DIR}/libgflags.a
    ADDLIB ${XXHASH_LIB_DIR}/libxxhash.a
    ADDLIB ${BOOST_LIB_DIR}/libboost_locale.a
    ADDLIB ${BOOST_LIB_DIR}/libboost_date_time.a
    ADDLIB ${BOOST_LIB_DIR}/libboost_filesystem.a
    ADDLIB ${YAMLCPP_LIB_DIR}/libyaml-cpp.a
    ADDLIB ${FMT_LIB_DIR}/libfmt.a
    ADDLIB ${MIMALLOC_LIB_DIR}/libmimalloc.a
    SAVE
    END
EOM
")
else()
execute_process(COMMAND bash -c "
ar -M << EOM
    CREATE ${SOLUTION_ROOT_DIR}/lib/libbqstgeng-cxx.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqstgeng.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqalgo.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqstgengimpl.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqposmgr.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqordmgr.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqpub.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libbqipc.a
    ADDLIB ${SOLUTION_ROOT_DIR}/lib/libpub.a
    ADDLIB ${CPR_LIB_DIR}/libcpr.a
    ADDLIB ${CURL_LIB_DIR}/libcurl.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_raw_hash_set.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_flags_reflection.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_hash.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_city.a
    ADDLIB ${ABSEIL_LIB_DIR}/libabsl_low_level_hash.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_binding_c.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_hoofs.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_platform.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh_config.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh_gateway.a
    ADDLIB ${ICEORYX_LIB_DIR}/libiceoryx_posh_roudi.a
    ADDLIB ${GFLAGS_LIB_DIR}/libgflags.a
    ADDLIB ${XXHASH_LIB_DIR}/libxxhash.a
    ADDLIB ${BOOST_LIB_DIR}/libboost_locale.a
    ADDLIB ${BOOST_LIB_DIR}/libboost_date_time.a
    ADDLIB ${BOOST_LIB_DIR}/libboost_filesystem.a
    ADDLIB ${YAMLCPP_LIB_DIR}/libyaml-cpp.a
    ADDLIB ${FMT_LIB_DIR}/libfmt.a
    ADDLIB ${MIMALLOC_LIB_DIR}/libmimalloc.a
    SAVE
    END
EOM
")
endif()
