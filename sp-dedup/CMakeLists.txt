cmake_minimum_required(VERSION 3.20)
project(sp_dedup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_DOCX_XLSX "Enable docx/xlsx processing (requires tinyxml2 + minizip)" ON)

# If using vcpkg, pass -DCMAKE_TOOLCHAIN_FILE=... on the cmake command line
find_package(tinyxml2 CONFIG QUIET)
find_package(unofficial-minizip CONFIG QUIET) # minizip from vcpkg often provides 'unofficial-minizip'

if (tinyxml2_FOUND AND unofficial-minizip_FOUND AND ENABLE_DOCX_XLSX)
  message(STATUS "tinyxml2 and minizip found -> docx/xlsx features enabled")
  add_definitions(-DHAS_DOCX_XLSX)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/file_ops.cpp
    src/hasher_win.cpp
    src/zip_util.cpp
    src/docx_dedupe.cpp
    src/xlsx_dedupe.cpp
)

add_executable(sp_dedup ${SOURCES})

if (tinyxml2_FOUND)
  target_link_libraries(sp_dedup PRIVATE tinyxml2::tinyxml2)
endif()

if (unofficial-minizip_FOUND)
  target_link_libraries(sp_dedup PRIVATE unofficial::minizip::minizip)
endif()

if (WIN32)
  target_link_libraries(sp_dedup PRIVATE bcrypt)
endif()
