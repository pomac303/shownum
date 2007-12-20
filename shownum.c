/*
 * shownum
 * -------
 * Copyright 2005-2006 by David Gerber <dg@zapek.com>
 * Copyright 2005-2007 by Ian Kumlien <pomac@vapor.com>
 * A reimplementation of Oliver Wagner's shownum
 * because he lost the source.
 *
 */
#include <stdio.h>
#include <ctype.h>

#if !defined(_MSC_VER)
/*
 * Compile on some ANSI C99 standard compiler with headers to define things
 */

#include <stdint.h>
#include <inttypes.h>

/*
 * If we are missing stdint, let the compiler know what size we want
 */
#if !(defined(_STDINT_H) || defined(_STDINT_H_))
typedef unsigned long long uint64_t;
#endif

/*
 * If we are missing inttypes, let the compiler know what formats we want
 */
#if !(defined(_INTTYPES_H) || defined(_INTTYPES_H_))
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRId64 "lld"
#define PRIo64 "llo"
#endif	/* !(defined(_INTTYPES_H) || defined(_INTTYPES_H_)) */

#else
/*
 * We are compiling on the Microsoft compiler
 * Force feed it what it wants to hear
 */

typedef unsigned __int64 uint64_t;

#define PRIu64 "I64u"
#define PRIx64 "I64x"
#define PRId64 "I64d"
#define PRIo64 "I64o"

#endif /* !defined(_MSC_VER) */

#define VERSION "1.7"

#define FALSE 0
#define TRUE 1

#define GET_HI(x)	(((x) & (uint64_t)0xffffffff00000000) >> 32)
#define GET_LO(x)	((x) & (uint64_t)0x00000000ffffffff)

static const char usage_information[] = {
	"shownum " VERSION " - http://zapek.com/software/shownum/\n\n"
	"usage: shownum <val>\n"
	"where val is one of:\n"
	" - decimal (eg. 5)\n"
	" - hexadecimal (eg. 0x5)\n"
	" - ansi chars, 8 max (eg. foo)\n"
	" - binary, 64-bits max (eg. %%1101)\n"
	"negative numbers work too\n"
};

typedef enum {
	MODE_DEC,
	MODE_HEX,
	MODE_BIN,
	MODE_ASCII
} mode_t;

int main(int argc, char **argv)
{
	if (argc == 2)
	{
		mode_t mode	= MODE_DEC;
		int	neg		= FALSE,
			no_bytes	= 0,
			index	= 0;
		uint64_t val = 0, bit = 0;

		char	buf[65], *p = argv[1];

		/*
		 * Check for negative numbers.
		 */
		if (*p == '-')
		{
			neg = TRUE;
			p++;
		}

		/*
		 * Check for a possible hex value.
		 */
		if (*p == '0' && *(p + 1) == 'x')
		{
			mode = MODE_HEX;
			p++;
		}

		/*
		 * Check if this is binary.
		 */
		if (mode == MODE_DEC)
		{
			if (*p == '%')
			{
				mode = MODE_BIN;
				p++;
			}
		}

		do
		{
			if (mode == MODE_BIN)
			{
				if (!(*p == '1' || *p == '0'))
				{
					mode = MODE_ASCII;
					break;
				}
			}
			else if (mode == MODE_HEX)
			{
				if (!isxdigit(*p))
				{
					mode = MODE_ASCII;
					break;
				}
			}
			else
			{
				if (!isdigit(*p))
				{
					mode = MODE_ASCII;
					break;
				}
			}
		}
		while (*(++p));

		while (*p)
			p++;

		/*
		 * Now do the conversion.
		 */
		if (mode == MODE_HEX)
			(void)sscanf(argv[1], "%"PRIx64, &val);
		else if (mode == MODE_ASCII)
		{
			bit = 0;

			if (p - argv[1] > 8)
				p = argv[1] + 8;
			
			/* We cast to unsigned char since we don't want a "negative"
			 * sentence or character.
			 */
			for (val = 0; p >= argv[1] && bit < 64; bit += 8)
				val += ((uint64_t)*(unsigned char *)(--p) << bit);
		}
		else if (mode == MODE_BIN)
		{
			register char chr = 0;

			if (p - argv[1] > 64)
				p = argv[1] + 64;

			bit = 1;
			val = 0;

			while ((chr = *(--p)))
			{
				if (chr == '%')
					break;
				else if (chr == '1')
					val |= bit;

				bit <<= 1;
			}

			if (neg)
				val = ~val;
		}
		else
			(void)sscanf(argv[1], "%"PRIu64, &val);

		/*
		 * How many bytes of information do we have?
		 */
		for (index = 56; index > -1; index -= 8)
		{
			if (no_bytes || (((uint64_t)0xff << index) & val) >> index)
				no_bytes++;
		}
		/* We need atleast one byte (return zeroed data */
		if (!no_bytes)
			no_bytes++;

		/*
		 * Extract any printable characters (US ASCII)
		 */
		for (p = buf, index = (no_bytes -1) << 3; index > -1; index -= 8, p++)
		{
			if (!isprint((*p = (char)((((uint64_t)0xff << index) & val) >> index))))
				*p = '.';
			if (index == 32)
			{
				*(++p) = ' ';
				*(++p) = '|';
				*(++p) = ' ';
			}
		}
		*p++ = 0;

		printf(	"Number of bits used: %i (%i bytes)\n"
				"Dec: %-22"PRIu64, no_bytes << 3, no_bytes, val);
		if (GET_HI(val))
			printf( " --- hi: %-11"PRIu64" lo: %"PRIu64, GET_HI(val), GET_LO(val));
		printf("\n");

		printf(	"Hex: 0x%-20"PRIx64, val);
		if (GET_HI(val))
			printf(	" --- hi: 0x%-9"PRIx64" lo: 0x%"PRIx64, GET_HI(val), GET_LO(val));
		printf("\n");

		printf(	"Oct: %-22"PRIo64, val);
		if (GET_HI(val))
			printf(	" --- hi: %-11"PRIo64" lo: %"PRIo64, GET_HI(val), GET_LO(val));
		printf("\n");
		
		printf ("Asc: %s\n", buf);

		/*
		 * Extract the actual bit pattern
		 */
		index = no_bytes << 3;
		bit = (uint64_t)1 << (index -1);
		for (p = buf, index = 64 - index; index < 64; index++)
		{
			*p++ = (val & bit) ? '1' : '0';
			bit >>= 1;

			if (index == 31)
			{
				*p++ = ' ';
				*p++ = '|';
				*p++ = ' ';
			}
		}
		*p = '\0';

		printf("Bin: %s\n     ", buf);
		for (index = no_bytes; index; index--)
		{
			printf ("^---+---");
			if (index == 5)
				printf ("   ");

		}
		printf("\n");
	}
	else
		printf(usage_information);

	return (0);
}

