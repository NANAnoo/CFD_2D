cmake_minimum_required(VERSION 3.25)
project(CFD_2D VERSION 0.1.0)
set(CXX_STANDARD 20)

file(GLOB cpp_files
     "src/*.cpp"
)

add_executable(CFD_2D ${cpp_files} src/SmoothKernelIMPL.h src/SmoothKernels.h src/ThreadPool.h src/LineBoundary.h)

find_package(OpenGL REQUIRED)
target_link_libraries(CFD_2D PRIVATE opengl32)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(CFD_2D PRIVATE glfw)

if (CMAKE_HOST_APPLE)
include_directories("/opt/homebrew/opt/glfw/include")
file(GLOB LIBRARIES "/opt/homebrew/opt/glfw/lib/*.dylib")
message("LIBRARIES = ${LIBRARIES}")

target_link_libraries(CFD_2D "-framework OpenGL")
target_link_libraries(CFD_2D ${LIBRARIES})
set_target_properties(CFD_2D PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
endif()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
