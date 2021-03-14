set(CMAKE_CXX_STANDARD 17)

file(GLOB ENGINE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable (Vulkaneer ${ENGINE_FILES})

set_property(TARGET Vulkaneer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Vulkaneer>")

target_include_directories(Vulkaneer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
##target_compile_definitions(Vulkaneer PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED) 
target_precompile_headers(Vulkaneer PUBLIC "vk_types.h" "<unordered_map>" "<vector>" "<iostream>" "<fstream>" "<string>" )
target_link_libraries(Vulkaneer vkbootstrap vma glm tinyobjloader imgui stb_image spirv_reflect)
target_link_libraries(Vulkaneer Vulkan::Vulkan sdl2)

add_dependencies(Vulkaneer Shaders)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Vulkaneer)
add_custom_command(TARGET Vulkaneer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../libs/sdl2/lib/x64/SDL2.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
)

