cmake_minimum_required(VERSION 3.16)

enable_testing()

set(SAMPLE_SOURCES
    sample_login.c
    sample_friends.c
)

foreach(SAMPLE ${SAMPLE_SOURCES})
    get_filename_component(SAMPLE_NAME ${SAMPLE} NAME_WE)
    add_executable(${SAMPLE_NAME} ${SAMPLE})
    target_link_libraries(${SAMPLE_NAME} PRIVATE steamkit_c)
endforeach()
