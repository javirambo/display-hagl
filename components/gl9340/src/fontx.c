/*

 MIT License

 Copyright (c) 2012 ChaN
 Copyright (c) 2020-2021 Mika Tuupola

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 -cut-

 This file is part of the HAGL graphics library:
 https://github.com/tuupola/hagl


 Adapted from the blog post by ChaN:
 http://elm-chan.org/docs/dosv/fontx_e.html

 SPDX-License-Identifier: MIT

 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "fontx.h"

const char *TAG = "fontx";

uint8_t* fontx_meta(fontx_meta_t *meta, const uint8_t *font)
{
	meta->width = font[FONTX_WIDTH];
	meta->height = font[FONTX_HEIGHT];
	meta->type = font[FONTX_TYPE];
	return (uint8_t*) font;
}

uint8_t fontx_glyph(fontx_glyph_t *glyph, wchar_t code, const uint8_t *font)
{
	uint32_t nc, bc, sb, eb;
	const uint8_t *block_table;

	fontx_meta_t meta;
	fontx_meta(&meta, font);

	glyph->width = meta.width;
	glyph->height = meta.height;
	glyph->pitch = (meta.width + 7) / 8;
	glyph->size = glyph->pitch * meta.height;

	//	The font size becomes (WF + 7) / 8 * HF [bytes].
	//	The offset where the top of font image data from top of the file are:
	//	Single byte code: 17 + Character code * Font size
	//	Double byte code: 18 + 4 * NB + Nunber of previous codes * Font size

	//  single-byte character set
	if (FONTX_TYPE_SBCS == meta.type)
	{
		if (code < 0x100)
		{
			glyph->buffer = &font[FONTX_GLYPH_DATA_START + code * glyph->size];
			return FONTX_OK;
		}
	}
	//  double-byte character set
	else
	{
		block_table = &font[FONTX_BLOCK_TABLE_START];
		nc = 0;
		bc = font[FONTX_BLOCK_TABLE_SIZE];
		while (bc--)
		{
			/* Get range of the code block_table. */
			sb = block_table[0] + block_table[1] * 0x100;
			eb = block_table[2] + block_table[3] * 0x100;

			/* Check if in the code block_table. */
			if (code >= sb && code <= eb)
			{
				/* Number of codes from top of the block_table. */
				nc += code - sb;
				glyph->buffer = &font[ FONTX_BLOCK_TABLE_START +
						4 * font[FONTX_BLOCK_TABLE_SIZE] +
						nc * glyph->size];
				return FONTX_OK;
			}
			/* Number of codes in the previous block_tables. */
			nc += eb - sb + 1;
			/* Next code block_table. */
			block_table += 4;
		}
	}

	ESP_LOGE(TAG, "GLYPH NOT FOUND [%c]", (char )code);
	return FONTX_ERR_GLYPH_NOT_FOUND;
}
