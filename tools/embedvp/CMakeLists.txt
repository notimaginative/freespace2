
add_executable(
	createvp
	EXCLUDE_FROM_ALL
	${CMAKE_CURRENT_SOURCE_DIR}/createvp.cpp
)

target_link_libraries(
	createvp
	SDL3::SDL3
)

# override runtime output path so we don't polute the real one with build tools
set_target_properties(createvp PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
)

set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist")

# NOTE: "embed_root" must match name of same folder in dist dir
set(EMBED_ROOT "${CMAKE_CURRENT_BINARY_DIR}/embed_root")
set(EMBED_FILE "${CMAKE_CURRENT_BINARY_DIR}/embed.vp")

add_custom_command(
	OUTPUT
		${EMBED_FILE}
	COMMAND ${CMAKE_COMMAND}
		-D CREATEVP=$<TARGET_FILE:createvp>
		-D DIST_DIR=${DIST_DIR}
		-D EMBED_ROOT=${EMBED_ROOT}
		-D EMBED_FILE=${EMBED_FILE}
		-D FS1=${FS1}
		-D RUNTIME_DIR=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/createvp.cmake
	DEPENDS
		createvp
		# add all embed files we need to track changes for below
		${DIST_DIR}/images/FS.png
		${DIST_DIR}/images/FS2.png
		${DIST_DIR}/embed_root/data/fonts/Cousine-Regular.ttf
		${DIST_DIR}/embed_root/data/fonts/DroidSans.ttf
		${DIST_DIR}/embed_root/data/interface/launch_fs1_bg.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_bg.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_help.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_help1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_help2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_play.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_play1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_play2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_pxo.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_pxo1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_pxo2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_quit.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_quit1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_quit2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_readme.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_readme1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_readme2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_setup.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_setup1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_setup2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_uninstall.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_uninstall1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_uninstall2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_update.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_update1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_update2.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_volition.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_volition1.png
		${DIST_DIR}/embed_root/data/interface/launch_fs2_volition2.png
		${DIST_DIR}/embed_root/data/shaders/aabitmap.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/color_fog.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/color_fog.vert.glsl
		${DIST_DIR}/embed_root/data/shaders/color.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/color.vert.glsl
		${DIST_DIR}/embed_root/data/shaders/nondark_fog.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/nondark.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/tex_fog.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/tex_fog.vert.glsl
		${DIST_DIR}/embed_root/data/shaders/tex.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/tex.vert.glsl
		${DIST_DIR}/embed_root/data/shaders/window.frag.glsl
		${DIST_DIR}/embed_root/data/shaders/window.vert.glsl
		${DIST_DIR}/embed_root/data/sounds/launch_fs2_hover.wav
		${DIST_DIR}/embed_root/data/sounds/launch_fs2_click.wav
		${DIST_DIR}/embed_root/data/text/launch_help.txt
	COMMENT "Creating embed.vp ..."
	VERBATIM
)

add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/embedvp.h
		${CMAKE_CURRENT_BINARY_DIR}/embedvp.cpp
	COMMAND ${CMAKE_COMMAND}
		-D SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-D BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
		-D EMBED_FILE=${EMBED_FILE}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/vp2cpp.cmake
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/embedvp.h.in
		${CMAKE_CURRENT_SOURCE_DIR}/embedvp.cpp.in
		${EMBED_FILE}
	COMMENT "Converting embed.vp to source file ..."
	VERBATIM
)

source_group("Source Files\\embedvp" FILES
	embedvp.cpp.in
)

source_group("Header Files\\embedvp" FILES
	embedvp.h.in
)

add_library(
	embedvp
	EXCLUDE_FROM_ALL
	${CMAKE_CURRENT_BINARY_DIR}/embedvp.cpp
	${CMAKE_CURRENT_BINARY_DIR}/embedvp.h
)

set_target_properties(embedvp PROPERTIES
	ADDITIONAL_CLEAN_FILES "${EMBED_ROOT};${EMBED_FILE}"
)

target_include_directories(embedvp PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
