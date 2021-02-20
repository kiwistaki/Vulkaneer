find_package(Vulkan REQUIRED)

add_library(vkbootstrap STATIC)
add_library(glm INTERFACE)
add_library(vma INTERFACE)
add_library(stb_image INTERFACE)
add_library(tinyobjloader STATIC)
add_library(spirv_reflect STATIC)

add_subdirectory(gltf)

target_sources(vkbootstrap PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap/VkBootstrap.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap/VkBootstrap.cpp"
)
target_sources(spirv_reflect PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/spv_reflect/spirv_reflect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/spv_reflect/spirv_reflect.c"
)
target_include_directories(spirv_reflect PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/spv_reflect" )
target_include_directories(spirv_reflect PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/spv_reflect/include" )
target_include_directories(vma INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vma" )
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm" )
target_include_directories(vkbootstrap PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vkbootstrap" )
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan)

## tinyobjloader
target_sources(tinyobjloader PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader/tiny_obj_loader.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader/tiny_obj_loader.cc"
)
target_include_directories(tinyobjloader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader" )

## SDL
add_library(sdl2 INTERFACE)
target_include_directories(sdl2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/include" )
target_link_directories(sdl2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/sdl2/lib/x64")
target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)

## Imgui
add_library(imgui STATIC)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
target_sources(imgui PRIVATE 
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_stdlib.h"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_stdlib.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_vulkan.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdl.cpp"
)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2)

## stb_image
target_include_directories(stb_image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/stb_image" )
