
set(imgui_SOURCE
	imconfig.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	imgui.cpp
	imgui.h
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
	backends/imgui_impl_opengl2.cpp
	backends/imgui_impl_opengl2.h
	backends/imgui_impl_sdl3.cpp
	backends/imgui_impl_sdl3.h
	backends/imgui_impl_sdlrenderer3.cpp
	backends/imgui_impl_sdlrenderer3.h
)

add_library(imgui STATIC ${imgui_SOURCE})

target_include_directories(imgui SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(imgui PUBLIC SDL3::SDL3)