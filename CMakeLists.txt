cmake_minimum_required (VERSION 3.8)

set(PROJECT "NiCeBar")
set(TARGET "ncbar")
project(${PROJECT})

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Enable compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(NORMAL_PATH PROJECT_DIR)
set(SOURCE_DIR "${PROJECT_DIR}/src")
set(INCLUDE_DIR "${PROJECT_DIR}/include")
set(SUBMODULES_DIR "${PROJECT_DIR}/submodules")
set(THIRDPARTY_DIR "${PROJECT_DIR}/thirdparty")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_DIR}/bin)

file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADERS "${SOURCE_DIR}/*.hpp" "${SOURCE_DIR}/*.h" "${INCLUDE_DIR}/*.hpp" "${INCLUDE_DIR}/*.h")

# Set their directories
foreach (SOURCE_FILE ${SOURCES};${HEADERS})

	# Get relative path
	cmake_path(RELATIVE_PATH SOURCE_FILE BASE_DIRECTORY ${PROJECT_DIR})

	# Extract directory
	get_filename_component(SOURCE_FILE_DIR ${SOURCE_FILE} DIRECTORY)
	
	# Set source group
	if (WIN32) # Replace slashes with backslashes for Windows
		string(REPLACE "/" "\\" SOURCE_FILE_DIR ${SOURCE_FILE_DIR})
	endif ()
	source_group("${SOURCE_FILE_DIR}" FILES ${SOURCE_FILE})

endforeach ()

# Thirdparty
set(XDG_SHELL_DIR "${THIRDPARTY_DIR}/xdg-shell/include")
# Submodules
set(ARGPARSE_DIR "${SUBMODULES_DIR}/argparse/include")

# Set include directories
set(INCLUDE_DIR
	"${SOURCE_DIR}/"
	"${INCLUDE_DIR}/"
	"${XDG_SHELL_DIR}/"
	"${ARGPARSE_DIR}/"
	)

add_executable(${TARGET} ${SOURCES} ${HEADERS})
target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")

	message( FATAL_ERROR "Sorry, bruh, this project is meant to be build only for Linux/Wayland" )

else ()

	if (WIN32)
		message( FATAL_ERROR "Sorry, bruh, this project is meant to be build only for Linux/Wayland" )
	endif ()

	add_subdirectory("${THIRDPARTY_DIR}/wlr-protocols")
	target_link_libraries(${TARGET} wlr-protocols vulkan wayland-client)

endif ()

include_directories(${INCLUDE_DIR})
add_definitions(-DTARGET="${TARGET}")

# Hide ZERO_CHECK and ALL_BUILD targets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER meta)
