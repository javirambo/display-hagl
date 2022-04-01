/*
 * rect.h
 *
 *  Created on: 15 mar. 2022
 *      Author: Javier
 */
#include "rect.h"

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
	rec->w = rec->x1 - rec->x0;
	rec->h = rec->y1 - rec->y0;
}

void set_rect_w(int x, int y, int w, int h, RECT *rec)
{
	rec->x0 = x;
	rec->y0 = y;
	rec->x1 = x + w - 1;
	rec->y1 = y + h - 1;
	rect_norm(rec);
	rec->w = w;
	rec->h = h;
}

void set_rect(int w, int h, RECT *rec)
{
	set_rect_w(0, 0, w, h, rec);
}

void grow_rect_w(RECT *rec, int size)
{
	rec->x0 -= size;
	rec->x1 += size;
	rec->y0 -= size;
	rec->y1 += size;
	rec->w += 2;
	rec->h += 2;
}

void resize_rect(RECT *rec, int size)
{
	rec->x0 -= size;
	rec->x1 += size;
	rec->y0 -= size;
	rec->y1 += size;
	rec->w += size;
	rec->w += size;
	rec->h += size;
	rec->h += size;
}
