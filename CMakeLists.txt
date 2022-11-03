cmake_minimum_required(VERSION 3.12)
project(cpp_app)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/exe)

set(CMAKE_C_STANDARD 11) 
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -ffast-math")
set(CMAKE_C_FLAGS_RELEASE "-O3 -ffast-math -DNDEBUG -s -fomit-frame-pointer -march=native -funroll-loops -fno-PIE -no-pie -mfpmath=sse -m64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ffast-math -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -ffast-math -DNDEBUG -g")

include_directories(src)
include_directories(src/cpu)
include_directories(src/gw)


file(GLOB sourcefiles 
"src/*.h" "src/*.cpp"
"src/gw/*.h" "src/gw/*.cpp"
"src/cpu/*.h" "src/cpu/*.cpp"
"src/lib/*.h" "src/lib/*.cpp"
)
add_executable(${PROJECT_NAME} ${sourcefiles})

find_package(OpenGL)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} SDL2_image SDL2_ttf SDL2 SDL2main)

