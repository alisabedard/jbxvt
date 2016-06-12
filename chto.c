#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
	if (argc < 3)
		return 1;
	int index = atoi(argv[1]);
	long int l = strtol(argv[2], NULL, 16);
	uint32_t v = l;
	const uint8_t o = 5; // keep 3 most significant bits
	uint16_t r = (v & (0xff<<16))>>16;
	uint16_t g = (v & (0xff<<8))>>8;
	uint16_t b = v & 0xff;
	r>>=o;
	g>>=o;
	b>>=o;
	printf("\t[%d] = 0%o%o%o,\n", index, r, g, b);
	return 0;
}
