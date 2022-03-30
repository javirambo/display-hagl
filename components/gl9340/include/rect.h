/*
 * rect.h
 *
 *  Created on: 15 mar. 2022
 *      Author: Javier
 */

#ifndef COMPONENTS_GL9340_INCLUDE_RECT_H_
#define COMPONENTS_GL9340_INCLUDE_RECT_H_

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

void set_rect(int w, int h, RECT *rec);

void resize_rect(RECT *rec, int size);

#endif /* COMPONENTS_GL9340_INCLUDE_RECT_H_ */
