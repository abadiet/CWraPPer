cmake_minimum_required(VERSION 3.10)

project(cwrapper)

# Options
option(CWRAPPER_DEBUG "Debug mode" OFF)

# Executable
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp )
add_executable(${PROJECT_NAME} ${SOURCES})

# Flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(LIB_FIX -Wno-unused-parameter -Wno-reserved-macro-identifier
    -Wno-old-style-cast -Wno-sign-conversion -Wno-shorten-64-to-32
    -Wno-documentation-deprecated-sync -Wno-shadow-field-in-constructor
    -Wno-missing-variable-declarations -Wno-implicit-int-conversion
    -Wno-documentation-unknown-command -Wno-shadow-uncaptured-local
    -Wno-switch-enum -Wno-switch-default -Wno-comma
    -Wno-exit-time-destructors -Wno-shadow -Wno-extra-semi
    -Wno-shadow-field -Wno-reserved-identifier -Wno-missing-noreturn
    -Wno-documentation -Wno-cast-align -Wno-suggest-destructor-override
    -Wno-duplicate-enum -Wno-signed-enum-bitfield -Wno-global-constructors
    -Wno-newline-eof -Wno-weak-vtables
    -Wno-inconsistent-missing-destructor-override -Wno-float-equal
    -Wno-double-promotion -std=c++17 )
target_compile_options(${PROJECT_NAME} PRIVATE
$<$<CXX_COMPILER_ID:AppleClang>:-O3 -Wall -Wextra -Werror
    -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wconversion
    -Wunreachable-code -Wpedantic -Wshadow -Wwrite-strings
    -Wcast-qual -Wstrict-aliasing -Wpointer-arith -Wformat=2
    -Weverything -Wextra-semi -Wmissing-prototypes
    -Wunreachable-code-aggressive -Wimplicit-fallthrough
    -Wloop-analysis -Wrange-loop-analysis -Wcomma -Wmove
    -Wconditional-uninitialized -Wassign-enum
    -Wunused-exception-parameter -Wcovered-switch-default
    -Wno-c++98-compat -Wno-c++98-compat-pedantic
    -Wno-padded ${LIB_FIX}>
$<$<CXX_COMPILER_ID:GNU>:-O3 -Wall -Wextra -Werror -Wfloat-equal
    -Wundef -Wcast-align -Wwrite-strings -Wconversion
    -Wunreachable-code -Wpedantic -Wshadow -Wwrite-strings
    -Wcast-qual -Wstrict-aliasing -Wpointer-arith -Wformat=2
    -Wimplicit-fallthrough -Wno-padded ${LIB_FIX}>
)
if(CWRAPPER_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -DCWRAPPER_DEBUG)
endif()

# Dependencies
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(${PROJECT_NAME} PRIVATE clangTooling clangBasic
    clangASTMatchers clangFrontend ${llvm_libs})
