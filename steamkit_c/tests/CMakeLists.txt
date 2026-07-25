cmake_minimum_required(VERSION 3.16)

enable_testing()

set(TEST_SOURCES
    test_game_id.c
    test_steam_id.c
    test_crypto.c
    test_key_value.c
    test_msg.c
)

foreach(TEST ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST} NAME_WE)
    add_executable(${TEST_NAME} ${TEST})
    target_link_libraries(${TEST_NAME} PRIVATE steamkit_c)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
