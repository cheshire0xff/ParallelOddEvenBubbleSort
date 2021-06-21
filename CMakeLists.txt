cmake_minimum_required(VERSION 3.10)

project(mainWorld)

find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH} inc)

add_executable(main src/main.cpp)
target_link_libraries(main ${MPI_LIBRARIES})

set_property(TARGET main PROPERTY CXX_STANDARD 17)
target_compile_options(main PRIVATE -Wall -Wextra -pedantic -Werror)

if(MPI_COMPILE_FLAGS) 
	set_target_properties(main PROPERTIES
		      COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
	set_target_properties(main PROPERTIES
	    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

