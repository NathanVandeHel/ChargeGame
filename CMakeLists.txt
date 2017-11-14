cmake_minimum_required(VERSION 3.5)

set(PNAME ChargeGame)
project(${PNAME})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if ( MSVC )
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	#set(SDL_DIR "./SDL-1.2.15")
endif()

add_definitions(-Wall)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(SDL REQUIRED)

include_directories(${SDL_INCLUDE_DIRS})

add_executable(${PNAME} ChargeGame.c)
set(MATHLIB m)
if ( MSVC )
	set(MATHLIB "")
endif()
target_link_libraries(${PNAME} ${SDL_LIBRARIES} ${MATHLIB})