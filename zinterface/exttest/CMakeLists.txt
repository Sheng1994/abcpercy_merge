# 设置最低版本要求
cmake_minimum_required(VERSION 3.12)

# 定义项目名称和语言
project(testmain LANGUAGES C)

# 指定源文件
set(SOURCE_FILES testmain.c)

# 定义可执行目标
add_executable(testmain ${SOURCE_FILES} ../cnf_gen.cpp)
target_link_libraries(testmain PRIVATE percy)

add_library(percylinklib ../cnf_gen.cpp)
#target_link_libraries(testmain percylinklib)
#target_link_libraries(testmain percy)

#target_include_directories(testmain PRIVATE ../../../include)