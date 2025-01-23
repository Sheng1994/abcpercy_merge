add_executable(vec_test vec/vec_test.cc)
include_directories(${CMAKE_SOURCE_DIR}/src)
target_link_libraries(vec_test PRIVATE libabc)