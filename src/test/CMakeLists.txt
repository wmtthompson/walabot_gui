project(unit_tests CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
#-------------------------------------------------------------------------------
# EXTERNAL PROJECT
#-------------------------------------------------------------------------------
# find thread library 
find_package(Threads REQUIRED)
include(ExternalProject)

set(WALABOT_FOLDER /home/william/walabot)
#SOURCE_DIR "${WALABOT_FOLDER}/googletest"
ExternalProject_add(googletest
	PREFIX ${WALABOT_FOLDER}/googletest
	SOURCE_DIR "${WALABOT_FOLDER}/googletest"
	BINARY_DIR "${WALABOT_FOLDER}/googletest"
	INSTALL_COMMAND "")

add_library(gtest STATIC IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION "${WALABOT_FOLDER}/googletest/lib/libgtest.a")

include_directories("${WALABOT_FOLDER}/googletest/googletest/include")

#-------------------------------------------------------------------------------
#	SOURCE CODE
#-------------------------------------------------------------------------------



add_executable(${PROJECT_NAME} test_main.cpp)
target_link_libraries(${PROJECT_NAME} gtest Threads::Threads)