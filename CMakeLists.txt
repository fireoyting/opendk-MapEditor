cmake_minimum_required(VERSION 3.14)
project(MapEditor VERSION 1.2.0)

set(CMAKE_BUILD_PARALLEL_LEVEL 12)


add_subdirectory("ImGui")
set_target_properties(ImGui PROPERTIES FOLDER "ImGui")
add_subdirectory("maplib")
set_target_properties(MPLIB PROPERTIES FOLDER "maplib")
add_subdirectory("FRLIB")
set_target_properties(FR PROPERTIES FOLDER "FRLIB")
add_subdirectory("SPLIB")
set_target_properties(SP PROPERTIES FOLDER "SPLIB")
add_subdirectory("COMMON")
set_target_properties(Common PROPERTIES FOLDER "COMMON")
add_subdirectory("src")



if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
