cmake_minimum_required(VERSION 3.28)
project(rhhashmap C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(rhhashmap STATIC src/rhhashmap.c
)

if (MSVC)
    target_compile_options(rhhashmap PRIVATE /W4 /WX)
else ()
    target_compile_options(rhhashmap PRIVATE -Wall -Wextra -Werror)
endif ()

include(FetchContent)
FetchContent_Declare(
        rapidhash
        URL https://github.com/Nicoshev/rapidhash/archive/refs/tags/rapidhash_v3.tar.gz
        URL_HASH SHA256=2e708bb2be76fd76f1e5d75bb3145d058e2dad0c5b0022646f84ec83aeff400d
)
FetchContent_MakeAvailable(rapidhash)
add_library(rapidhash INTERFACE)
target_include_directories(rapidhash INTERFACE
        ${rapidhash_SOURCE_DIR}
)

target_link_libraries(rhhashmap PRIVATE rapidhash)

install(TARGETS rhhashmap ARCHIVE DESTINATION lib)
install(FILES src/rhhashmap.h DESTINATION include)

include(CTest)

if (BUILD_TESTING)
    FetchContent_Declare(
            cmocka
            URL https://cmocka.org/files/2.0/cmocka-2.0.2.tar.xz
            URL_HASH SHA256=39f92f366bdf3f1a02af4da75b4a5c52df6c9f7e736c7d65de13283f9f0ef416
            EXCLUDE_FROM_ALL
    )
    set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(UNIT_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cmocka)

    add_executable(test_rhhashmap
            tests/test_rhhashmap.c)
    target_link_libraries(test_rhhashmap PRIVATE rhhashmap cmocka)
    add_custom_command(TARGET test_rhhashmap POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cmocka>
            $<TARGET_FILE_DIR:test_rhhashmap>
    )

    add_test(NAME rhhashmap_tests COMMAND test_rhhashmap)
endif ()
