idf_component_register(
        SRCS "src/bitmap.c" "src/clip.c" "src/fontx.c" "src/colors.c" "src/jpeg_decomp.c" "src/gl.c" "src/rect.c"
        	"src/hal/hagl_hal_double.c" "src/hal/hagl_hal_single.c" "src/hal/hagl_hal_triple.c" "src/hal/mipi_display.c"
        INCLUDE_DIRS "./include" 
        REQUIRES "fs_tools"
    )
