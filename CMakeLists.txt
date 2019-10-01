
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
add_executable(m m.c)
target_link_libraries(m dl)

add_library(mm SHARED mm.c)
