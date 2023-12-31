cmake_minimum_required(VERSION 3.26.3)
# set (CMAKE_GENERATOR_PLATFORM Win32)

project(skeleton-animation)
set (CMAKE_CXX_STANDARD 20)


# Add .lib files
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/src/*.h
	${CMAKE_SOURCE_DIR}/src/*.hpp)

if(WIN32)
	file(RELATIVE_PATH ROOT_DIR
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
		${CMAKE_SOURCE_DIR})
else()
	file(RELATIVE_PATH ROOT_DIR
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR})
endif()



# message(STATUS "root related path" "${CMAKE_CURRENT_BINARY_DIR}/")
message(STATUS "root related path" "${ROOT_DIR}")
# Configure assets header file
configure_file(src/render/cmake-source-dir.hpp.in src/render/cmake-source-dir.hpp)
include_directories(${CMAKE_BINARY_DIR}/src)
	
# Define the executable
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# We need a CMAKE_DIR with some code to find external dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#######################################
# LOOK for the packages that we need! #
#######################################

# OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# GLM
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

# GLFW
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")


# Imgui
# find_package(IMGUI REQUIRED)
# message(STATUS "Found IMGUI in ${IMGUI_INCLUDE_DIR}")
file(GLOB_RECURSE IMGUI_SOURCE ${CMAKE_SOURCE_DIR}/src/imgui/*.cpp)
add_library(IMGUI ${IMGUI_SOURCE})

# ASSIMP
find_package(ASSIMP REQUIRED)
message(STATUS "Found ASSIMP in ${ASSIMP_INCLUDE_DIR}")

# GLEW
find_package(GLEW REQUIRED)
message(STATUS "Found GLEW in ${GLEW_INCLUDE_DIR}")

# Nlohmann
find_package(NLOHMANN REQUIRED)
message(STATUS "Found NLOHMANN in ${NLOHMANN_INCLUDE_DIR}")

# STB_IMAGE
add_library(STB_IMAGE "src/stb-image/stb_image.cpp")

# # GLAD
# add_library(GLAD "src/thirdparty/glad.c")

# Put all libraries into a variable
# set(LIBS ${GLFW3_LIBRARY} ${OPENGL_LIBRARY} GLAD ${CMAKE_DL_LIBS} ${ASSIMP_LIBRARY} STB_IMAGE)
set(LIBS ${GLFW3_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS} ${ASSIMP_LIBRARY} ${GLEW_LIBRARY} IMGUI STB_IMAGE)

# Define the include DIRs
include_directories(
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/include"
	# "${CMAKE_SOURCE_DIR}/include/glad"
	"${CMAKE_SOURCE_DIR}/include/glm"
)

# Define the link libraries
target_link_libraries(${PROJECT_NAME} ${LIBS})

# Create virtual folders to make it look nicer in VS
if(MSVC_IDE)
	# Macro to preserve source files hierarchy in the IDE
	macro(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)

		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else()
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src" "Sources" groupname ${groupname})
				source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endmacro()

	# Run macro
	GroupSources(src)
endif()

# Copy dlls
if(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/bin"
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
else()
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"${PROJECT_SOURCE_DIR}/lib/*.so"
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()# <--this is out-file path