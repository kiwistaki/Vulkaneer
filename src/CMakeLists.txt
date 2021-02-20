set(CMAKE_CXX_STANDARD 17)

file(GLOB ENGINE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable (QuestEngine ${ENGINE_FILES})

set_property(TARGET QuestEngine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:QuestEngine>")

target_include_directories(QuestEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(QuestEngine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED) 
target_precompile_headers(QuestEngine PUBLIC "vk_types.h" "<unordered_map>" "<vector>" "<iostream>" "<fstream>" "<string>" )
target_link_libraries(QuestEngine vkbootstrap vma glm tinyobjloader imgui stb_image spirv_reflect)
target_link_libraries(QuestEngine Vulkan::Vulkan sdl2)

add_dependencies(QuestEngine Shaders)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT QuestEngine)
add_custom_command(TARGET QuestEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../libs/sdl2/lib/x64/SDL2.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
)

