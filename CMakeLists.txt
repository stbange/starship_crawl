cmake_minimum_required(VERSION 3.22)
project(starship_crawl VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(EMSCRIPTEN)
  # Emscripten's EM_ASM bridge (also used by SDL) requires GNU C extensions.
  set(CMAKE_C_EXTENSIONS ON)
endif()
option(SC_BUILD_GAME "Build the SDL/OpenGL application" ON)
option(SC_SANITIZE "Enable address and undefined behavior sanitizers" OFF)

add_library(sc_core src/content.c src/generation.c src/world.c src/progression.c src/game.c src/save.c)
set_target_properties(sc_core PROPERTIES C_EXTENSIONS OFF)
target_include_directories(sc_core PUBLIC src)
if(NOT MSVC)
  target_compile_options(sc_core PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(sc_core PUBLIC m)
endif()
if(SC_SANITIZE AND NOT MSVC AND NOT EMSCRIPTEN)
  target_compile_options(sc_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(sc_core PUBLIC -fsanitize=address,undefined)
endif()

include(CTest)
if(BUILD_TESTING AND NOT EMSCRIPTEN)
  add_executable(sc_tests tests/test_core.c)
  target_link_libraries(sc_tests PRIVATE sc_core)
  target_compile_definitions(sc_tests PRIVATE SC_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME core COMMAND sc_tests)
endif()

if(SC_BUILD_GAME)
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    URL https://codeload.github.com/libsdl-org/SDL/tar.gz/f5e5f6588921eed3d7d048ce43d9eb1ff0da0ffc
    URL_HASH SHA256=7f80bce51b7179bab472e75b90ce8ac06b162b3da8e19feadecee5acea07dbfb
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(SDL3)
  add_executable(starship_crawl src/main.c src/render.c src/gl_api.c)
  target_link_libraries(starship_crawl PRIVATE sc_core SDL3::SDL3)
  if(NOT MSVC)
    target_compile_options(starship_crawl PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  if(EMSCRIPTEN)
    set_target_properties(starship_crawl PROPERTIES SUFFIX ".html")
    target_link_options(starship_crawl PRIVATE
      "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2"
      "-sALLOW_MEMORY_GROWTH=1" "-sINITIAL_MEMORY=67108864" "-sMAXIMUM_MEMORY=268435456"
      "-sMINIFY_HTML=0"
      "-sEXPORTED_FUNCTIONS=['_main','_sc_web_start','_sc_web_save','_sc_web_state','_sc_web_action','_sc_web_import','_sc_web_fingerprint']"
      "-sEXPORTED_RUNTIME_METHODS=['ccall','FS']" "-sFORCE_FILESYSTEM=1"
      "-lidbfs.js" "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
      "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/web/storage.js")
    set_property(TARGET starship_crawl APPEND PROPERTY LINK_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html ${CMAKE_CURRENT_SOURCE_DIR}/web/storage.js)
  endif()
endif()
