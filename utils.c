#include <stdio.h>
#include <stdint.h>

#include "utils.h"

void printdosdate(const uint16_t date, const uint16_t time)
{
	uint32_t ds = (date << 16) | time;

	uint16_t y = ((ds >> 25) & 127) + 1980;
	uint8_t m = (ds >> 21) & 15;
	uint8_t d = (ds >> 16) & 31;
	uint8_t hh = (ds >> 11) & 31;
	uint8_t mm = (ds >> 5) & 63;
	uint8_t ss = ((ds) & 31) * 2;

	fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, hh, mm, ss);
}
