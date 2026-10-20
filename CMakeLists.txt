cmake_minimum_required (VERSION 4.4)

set(PROJECT "NCBar")
set(TARGET "ncbar")
project(${PROJECT})

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# Enable compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prepare directories
set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
cmake_path(NORMAL_PATH PROJECT_DIR)
set(SOURCE_DIR "${PROJECT_DIR}/src")
set(INCLUDE_DIR "${PROJECT_DIR}/include")
set(SUBMODULES_DIR "${PROJECT_DIR}/submodules")
set(THIRDPARTY_DIR "${PROJECT_DIR}/thirdparty")

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_DIR}/bin)

# Scan for source files
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
set(WAYLAND_PROTOCOLS_DIR "${THIRDPARTY_DIR}/wlr-protocols/include")
include(FetchContent)
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG        144d1e7c29b3b0c6dede7917a0476cc95248559c
    GIT_SUBMODULES ""
)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG        a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d
    GIT_SUBMODULES ""
)
set(CGLM_SHARED OFF CACHE BOOL "" FORCE)
set(CGLM_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cglm stb)
add_library(stb_headers INTERFACE)
target_include_directories(stb_headers SYSTEM INTERFACE "${stb_SOURCE_DIR}")
# Submodules
# set(CGLM_DIR "${SUBMODULES_DIR}/cglm")
# set(CGLM_INCLUDE_DIR "${CGLM_DIR}/include")
# set(STB_DIR "${SUBMODULES_DIR}/stb")

# Set include directories
set(INCLUDE_DIR
	"${SOURCE_DIR}/"
	"${INCLUDE_DIR}/"
	"${WAYLAND_PROTOCOLS_DIR}/"
	# "${CGLM_INCLUDE_DIR}/"
	# "${STB_DIR}/"
	)

add_executable(${TARGET} ${SOURCES} ${HEADERS})
#target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
# add_subdirectory("${CGLM_DIR}")

find_package(Vulkan)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

	add_subdirectory("${THIRDPARTY_DIR}/wlr-protocols")
	target_link_libraries(${TARGET} m wlr-protocols wayland-client cglm stb_headers)

else ()

	message( FATAL_ERROR "Sorry, bruh, this project is meant to be build only for Linux/Wayland" )

endif ()

target_include_directories(${TARGET} PRIVATE ${INCLUDE_DIR})
add_definitions(-DTARGET="${TARGET}")

# Hide ZERO_CHECK and ALL_BUILD targets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER meta)
