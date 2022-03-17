/*
 * rect.h
 *
 *  Created on: 15 mar. 2022
 *      Author: Javier
 */
#include "../../gl9340/include/rect.h"

static void swap_int(int *a, int *b)
{
	int c = *a;
	*a = *b;
	*b = c;
}

// normaliza el rectangulo
void rect_norm(RECT *rec)
{
	if (rec->x0 > rec->x1)
		swap_int(&rec->x0, &rec->x1);
	if (rec->y0 > rec->y1)
		swap_int(&rec->y0, &rec->y1);
}

void set_rect_c(int x0, int y0, int x1, int y1, RECT *rec)
{
	rec->x0 = x0;
	rec->x1 = x1;
	rec->y0 = y0;
	rec->y1 = y1;
	rect_norm(rec);
	rec->w = rec->x1 - rec->x0 + 1;
	rec->h = rec->y1 - rec->y0 + 1;
}

void set_rect_w(int x, int y, int w, int h, RECT *rec)
{
	rec->x0 = x;
	rec->y0 = y;
	rec->x1 = x + w;
	rec->y1 = y + h;
	rect_norm(rec);
	rec->w = rec->x1 - rec->x0 + 1;
	rec->h = rec->y1 - rec->y0 + 1;
}
