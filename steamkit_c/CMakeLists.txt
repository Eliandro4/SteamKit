cmake_minimum_required(VERSION 3.16)
project(steamkit_c C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

# Required dependencies
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)

# Optional dependencies
option(SK_ENABLE_LZMA "Enable LZMA support" OFF)
option(SK_ENABLE_ZSTD "Enable Zstandard support" OFF)
option(SK_BUILD_SAMPLES "Build samples" OFF)
option(SK_BUILD_TESTS "Build tests" OFF)

# Source files
set(SK_SOURCES
    src/types/game_id.c
    src/types/steam_id.c
    src/types/job_id.c
    src/types/global_id.c
    src/types/key_value.c
    src/types/msg_object.c
    src/types/depot_manifest.c
    src/types/workshop_file_type.c
    src/base/emsg.c
    src/base/msg_hdr.c
    src/base/packet_base.c
    src/base/client_msg.c
    src/networking/connection.c
    src/networking/tcp_connection.c
    src/networking/websocket_connection.c
    src/networking/udp_connection.c
    src/networking/envelope_encrypted_connection.c
    src/steam/cm_client.c
    src/steam/steam_client.c
    src/steam/callbacks.c
    src/steam/handlers/client_msg_handler.c
    src/steam/handlers/client_msg_protobuf.c
    src/steam/handlers/steam_friends.c
    src/steam/handlers/steam_user.c
    src/steam/handlers/steam_apps.c
    src/steam/handlers/steam_game_coordinator.c
    src/steam/handlers/steam_game_server.c
    src/steam/handlers/steam_user_stats.c
    src/steam/handlers/steam_master_server.c
    src/steam/handlers/steam_cloud.c
    src/steam/handlers/steam_workshop.c
    src/steam/handlers/steam_unified_messages.c
    src/steam/handlers/steam_screenshots.c
    src/steam/handlers/steam_matchmaking.c
    src/steam/handlers/steam_networking.c
    src/steam/handlers/steam_content.c
    src/steam/handlers/steam_auth_ticket.c
    src/utils/crypto_helper.c
    src/utils/debug_log.c
    src/utils/net_helpers.c
    src/utils/msg_util.c
    src/utils/http_client.c
    src/utils/adler32.c
    src/utils/lancache.c
    src/steam/steam_client/configuration/steam_configuration.c
    src/steam/steam_client/callback_mgr/callback_mgr.c
    src/steam/authentication/steam_authentication.c
    src/steam/services/cloud_service.c
    src/cdn/cdn_server.c
    src/cdn/cdn_client.c
    src/steam/handlers/steam_published_file.c
)

file(GLOB PROTOBUF_SOURCES "src/protobufs/*.c")
list(APPEND SK_SOURCES ${PROTOBUF_SOURCES})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protobufs
)

add_library(steamkit_c STATIC ${SK_SOURCES})

set_target_properties(steamkit_c PROPERTIES
    OUTPUT_NAME "steamkit"
    VERSION "3.0.0"
    SOVERSION 3
)

target_link_libraries(steamkit_c PUBLIC OpenSSL::SSL OpenSSL::Crypto CURL::libcurl ZLIB::ZLIB)
target_compile_definitions(steamkit_c PUBLIC SK_ENABLE_OPENSSL=1 SK_ENABLE_CURL=1 SK_ENABLE_ZLIB=1)

# Require protobuf-c for real networking
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF_C REQUIRED libprotobuf-c)
include_directories(${PROTOBUF_C_INCLUDE_DIRS})
target_link_libraries(steamkit_c PUBLIC ${PROTOBUF_C_LIBRARIES})
target_compile_definitions(steamkit_c PUBLIC SK_ENABLE_PROTOBUF_C=1)

# Build samples
if(SK_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# Build tests
if(SK_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Install rules
include(GNUInstallDirs)

install(TARGETS steamkit_c
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/steamkit/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/steamkit
    FILES_MATCHING PATTERN "*.h"
)

install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/steamkit_c)
