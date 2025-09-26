cmake_minimum_required(VERSION 3.12)
project(QThread LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(qthread INTERFACE)
target_include_directories(qthread INTERFACE include)

# tests
enable_testing()
add_subdirectory(tests)

# clang-format target
find_program(CLANG_FORMAT_EXE NAMES clang-format)

if (CLANG_FORMAT_EXE)
	file(GLOB_RECURSE ALL_CXX_HEADERS
		"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
	)

	add_custom_target(
		format
		COMMAND ${CLANG_FORMAT_EXE}
		-i
		${ALL_CXX_HEADERS}
		COMMENT "Running clang-format on all source files"
	)

	add_custom_target(
        format-check
        COMMAND ${CLANG_FORMAT_EXE}
        --dry-run
        --Werror
        ${ALL_CXX_HEADERS}
        COMMENT "Checking code formatting"
    )
endif()