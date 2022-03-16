/*
 * rect.h
 *
 *  Created on: 15 mar. 2022
 *      Author: Javier
 */

#ifndef COMPONENTS_HAGL_INCLUDE_RECT_H_
#define COMPONENTS_HAGL_INCLUDE_RECT_H_

typedef struct
{
	int x0;
	int y0;
	int x1;
	int y1;
	int w;
	int h;
} RECT; // rectangulo normalizado

// normaliza el rectangulo
void rect_norm(RECT *rec);

void set_rect_c(int x0, int y0, int x1, int y1, RECT *rec);

void set_rect_w(int x, int y, int w, int h, RECT *rec);

#endif /* COMPONENTS_HAGL_INCLUDE_RECT_H_ */
