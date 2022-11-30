cmake_minimum_required(VERSION 3.0.0)
project(CFD_2D VERSION 0.1.0)

file(GLOB cpp_files
     "src/*.cpp"
)

add_executable(CFD_2D ${cpp_files})
find_package(OpenGL REQUIRED)

if (CMAKE_HOST_APPLE)
include_directories("/opt/homebrew/opt/glfw/include")
file(GLOB LIBRARIES "/opt/homebrew/opt/glfw/lib/*.dylib")
message("LIBRARIES = ${LIBRARIES}")

target_link_libraries(CFD_2D "-framework OpenGL")
target_link_libraries(CFD_2D ${LIBRARIES})
set_target_properties(CFD_2D PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
