idf_component_register(
        SRCS "src/bitmap.c" "src/clip.c" "src/fontx.c" "src/colors.c" "src/jpeg_decomp.c" "src/rect.c"
        	"src/hal/hagl_hal_double.c" "src/hal/hagl_hal_single.c" "src/hal/mipi_display.c" "src/gl.c" 
        INCLUDE_DIRS "./include" 
        REQUIRES "fs_tools"
    )
