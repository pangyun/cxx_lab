cmake_minimum_required(VERSION 3.5)
project(DBLab)
set(CMAKE_CXX_STANDARD 17)

aux_source_directory(./ source_files)

include_directories(/usr/include)
include_directories(/usr/local/include)

link_directories(/usr/lib/gcc/x86_64-linux-gnu/5/
    /usr/lib/x86_64-linux-gnu
    /usr/lib
    /usr/local/lib
    )

set(src ${source_files})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fPIC")

add_executable(db_lab ${src})

set(lab_libs
    gflags
    pthread
    stdc++fs
    )

target_link_libraries(db_lab ${lab_libs})

enable_testing()
add_subdirectory(ut)
add_test(NAME test
         COMMAND ut/myut)
