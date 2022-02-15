/*
 * pig_resources.h
 *
 *  Created on: 10 feb. 2022
 *      Author: Javier
 */

#ifndef MAIN_PIG_RESOURCES_H_
#define MAIN_PIG_RESOURCES_H_

#include <stdint.h>

//-- archivos linkeados (se suben a la flash en el linker, y queda esta referencia para usarlos)
//-- agregar los archivos en CMakeLists.txt

#define BE(res) "_binary_"#res"_end"
#define BS(res) "_binary_"#res"_start"
#define RES_SIZE(res) BE(res)-BS(res)
#define EMBED_IMAGE(res) \
    extern const uint8_t res[] asm(BS(res)); \
    extern const uint8_t res##_end[] asm(BE(res));

EMBED_IMAGE(font_squid_30_bmp)

#endif /* MAIN_PIG_RESOURCES_H_ */
