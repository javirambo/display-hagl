#include <stdint.h>
#include <esp_log.h>
#include "colors.h"

hsl_t rgb888_to_hsl(rgb_t *rgb)
{
	hsl_t hsl;
	float r, g, b, h, s, l;
	r = rgb->r / 256.0;
	g = rgb->g / 256.0;
	b = rgb->b / 256.0;

	float maxColor = max(r, max(g, b));
	float minColor = min(r, min(g, b));

	/* R == G == B, so it's a shade of gray. */
	if (minColor == maxColor)
	{
		h = 0.0;
		s = 0.0;
		l = r;
	}
	else
	{
		l = (minColor + maxColor) / 2;

		if (l < 0.5)
		{
			s = (maxColor - minColor) / (maxColor + minColor);
		}
		else
		{
			s = (maxColor - minColor) / (2.0 - maxColor - minColor);
		}

		if (r == maxColor)
		{
			h = (g - b) / (maxColor - minColor);
		}
		else if (g == maxColor)
		{
			h = 2.0 + (b - r) / (maxColor - minColor);
		}
		else
		{
			h = 4.0 + (r - g) / (maxColor - minColor);
		}

		h /= 6; /* To bring it to a number between 0 and 1. */
		if (h < 0)
		{
			h++;
		}

	}

	hsl.h = (uint8_t) (h * 255.0);
	hsl.s = (uint8_t) (s * 255.0);
	hsl.l = (uint8_t) (l * 255.0);

	return hsl;
}

uint16_t rgb888_to_rgb565(rgb_t *input)
{
	uint16_t r5 = (input->r * 249 + 1014) >> 11;
	uint16_t g6 = (input->g * 253 + 505) >> 10;
	uint16_t b5 = (input->b * 249 + 1014) >> 11;

	return (r5 | g6 | b5);
}

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t rgb;

	rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
	rgb = (((rgb) << 8) & 0xFF00) | (((rgb) >> 8) & 0xFF);

	//ESP_LOGI(__FUNCTION__, "0x%04X // %d, %d, %d", rgb, r, g, b);
	return rgb;
}

rgb_t rgb565_to_rgb888(uint16_t *input)
{
	rgb_t rgb;

	uint8_t r5 = (*input & 0xf800) >> 8; // 1111100000000000
	uint8_t g6 = (*input & 0x07e0) >> 3; // 0000011111100000
	uint8_t b5 = (*input & 0x001f) << 3; // 0000000000011111

	rgb.r = (r5 * 527 + 23) >> 6;
	rgb.g = (g6 * 259 + 33) >> 6;
	rgb.b = (b5 * 527 + 23) >> 6;

	return rgb;
}

rgb_t hsl_to_rgb888(hsl_t *hsl)
{
	rgb_t rgb;
	float r, g, b, h, s, l;
	float temp1, temp2, tempr, tempg, tempb;

	h = hsl->h / 256.0;
	s = hsl->s / 256.0;
	l = hsl->l / 256.0;

	/* Saturation 0 means shade of grey. */
	if (s == 0)
	{
		r = g = b = l;
	}
	else
	{
		if (l < 0.5)
		{
			temp2 = l * (1 + s);
		}
		else
		{
			temp2 = (l + s) - (l * s);
		}
		temp1 = 2 * l - temp2;
		tempr = h + 1.0 / 3.0;
		if (tempr > 1)
		{
			tempr--;
		}
		tempg = h;
		tempb = h - 1.0 / 3.0;
		if (tempb < 0)
		{
			tempb++;
		}

		/* Red */
		if (tempr < 1.0 / 6.0)
		{
			r = temp1 + (temp2 - temp1) * 6.0 * tempr;
		}
		else if (tempr < 0.5)
		{
			r = temp2;
		}
		else if (tempr < 2.0 / 3.0)
		{
			r = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempr) * 6.0;
		}
		else
		{
			r = temp1;
		}

		/* Green */
		if (tempg < 1.0 / 6.0)
		{
			g = temp1 + (temp2 - temp1) * 6.0 * tempg;
		}
		else if (tempg < 0.5)
		{
			g = temp2;
		}
		else if (tempg < 2.0 / 3.0)
		{
			g = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempg) * 6.0;
		}
		else
		{
			g = temp1;
		}

		/* Blue */
		if (tempb < 1.0 / 6.0)
		{
			b = temp1 + (temp2 - temp1) * 6.0 * tempb;
		}
		else if (tempb < 0.5)
		{
			b = temp2;
		}
		else if (tempb < 2.0 / 3.0)
		{
			b = temp1 + (temp2 - temp1) * ((2.0 / 3.0) - tempb) * 6.0;
		}
		else
		{
			b = temp1;
		}
	}

	rgb.r = (uint8_t) (r * 255.0);
	rgb.g = (uint8_t) (g * 255.0);
	rgb.b = (uint8_t) (b * 255.0);

	return rgb;
}
