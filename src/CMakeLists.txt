cmake_minimum_required(VERSION 3.14)

add_compile_definitions(UNICODE _UNICODE)
if (WIN7_SYSTEM_SUPPORT MATCHES ON)
    add_compile_definitions(_WIN32_WINNT=0x601)
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

aux_source_directory(. DIR_SRCS)
#file(GLOB HLSL_FILES ../HLSL/*.hlsl ../HLSL/*.hlsli)
file(GLOB HEADER_FILES ./*.h)

#foreach(HLSL_FILE ${HLSL_FILES})



add_executable(MAPeditor WIN32 ${DIR_SRCS} ${HEADER_FILES} ${HLSL_FILES})
target_link_libraries(MAPeditor d3d11.lib dxgi.lib dwrite.lib d2d1.lib windowscodecs.lib dxguid.lib D3DCompiler.lib winmm.lib)
target_link_libraries(MAPeditor Common)
# ImGui
target_link_libraries(MAPeditor ImGui)
file(COPY imgui.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(MAPeditor FR)
target_link_libraries(MAPeditor SP)
target_link_libraries(MAPeditor MPLIB)
target_include_directories(MAPeditor PRIVATE ../maplib)
#source_group("HLSL Files" FILES ${HLSL_FILES})
set_target_properties(MAPeditor PROPERTIES OUTPUT_NAME "MapEditor")

set_target_properties(MAPeditor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(MAPeditor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(MAPeditor PROPERTIES FOLDER "MapEditor")

