cmake_minimum_required(VERSION 3.20)
project(sp_dedup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(tinyxml2 CONFIG REQUIRED)
find_package(unofficial-minizip CONFIG REQUIRED)

add_executable(sp_dedup
  src/main.cpp
  src/file_ops.cpp
  src/hasher_win.cpp
  src/zip_util.cpp
  src/xlsx_dedup.cpp
  src/docx_dedup.cpp
)

target_include_directories(sp_dedup PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(sp_dedup PRIVATE
  tinyxml2::tinyxml2
  unofficial::minizip::minizip
  bcrypt
)
