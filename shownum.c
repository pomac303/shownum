/*
 * shownum 2
 * ---------
 * Copyright 2008 by Ian Kumlien <pomac@vapor.com>
 *
 * A reimplementation of Oliver Wagner's shownum because he lost the source.
 * A reimplementation of David Gerber's shownum since it was hard to maintain.
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

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
#endif    /* !(defined(_INTTYPES_H) || defined(_INTTYPES_H_)) */

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

#define VERSION "2.0"

#define FALSE 0
#define TRUE 1

#define GET_HI(x)   ((x) >> 32)
#define GET_LO(x)   ((x) & (uint64_t)0x00000000ffffffff)

/* Endian, -le -be
 * --dec, --hex, --chr, --bin
 *  -d -h -c -b
 */

typedef enum
{
	ENDIAN_BIG,
	ENDIAN_LITTLE
} endian_t;

struct _parse_data_t;

typedef int (*parse_func_t)(unsigned char p, struct _parse_data_t *data);

typedef struct _parse_data_t
{
	parse_func_t parse;
	uint64_t value;
	int offset;
	int handle_neg;
	char name[15];
} parse_data_t;

static int parse_dec (unsigned char p, parse_data_t *data);
static int parse_hex (unsigned char p, parse_data_t *data);
static int parse_chr (unsigned char p, parse_data_t *data);
static int parse_bin (unsigned char p, parse_data_t *data);
static int parse_oct (unsigned char p, parse_data_t *data);

#if 0
#define DEBUG(x, y) printf("[DEBUG] %s name: %s value: %c\n", __FUNCTION__, (y), (x))
#else
#define DEBUG(x, y);
#endif

static int
parse_dec (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);
	if (isdigit(p))
	{
		data->value += (p - '0') * data->offset;
		data->offset *= 10;

		return 1;
	}
	else
		return 0;
}

static int
parse_hex (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);

	/* Hex can have a 0x prefix */
	switch (p)
	{
		case 'x':
			data->offset = ~0;
			return 1;
		case '0':
			if (data->offset == ~0)
				return 1;
	}

	if (isalnum(p) && toupper(p) < 'G' && data->offset != ~0)
	{
		data->value += (isdigit(p) ? (p - '0') : (toupper(p) - ('A' - 10))) << data->offset;
		data->offset += 4;

		return 1;
	}
	else
		return 0;
}

static int
parse_chr (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);

	/* Chars can be prefixed or suffixed by ' or " */
	switch (p)
	{
		case '\"':
		case '\'':
			return 1;
	}
	if (data->offset < 64)
	{
		data->value += ((uint64_t)p) << data->offset;
		data->offset += 8;
	}
	return 1;
}

static int
parse_bin (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);
	
	/* Binary data can be prefixed with % */
	if (p == '%')
		return 1;
	if (p == '0' || p == '1')
	{
		data->value += (p - '0') << data->offset;
		data->offset += 1;

		return 1;
	}
	else
		return 0;
}

static int
parse_oct (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);
	(void)p;
	(void)data;
	return 0;
}

static const char usage_information[] = {
	"shownum " VERSION " - http://pomac.netswarm.net/misc/\n\n"
	"usage: shownum <val>\n"
	"where val is one of:\n"
	" - decimal (eg. 5)\n"
	" - hexadecimal (eg. 0x5)\n"
	" - ascii chars, 8 max (eg. foo)\n"
	" - binary, 64-bits max (eg. 1101)\n"
	"negative numbers work too\n"
	"\noptions:\n"
	" -a -- display all matches\n"
	"\n  endianess (input or output):\n"
	"   -le -- little endian\n"
	"   -be -- big endian (default)\n"
	"\nThe result will be presented in all possible\n"
	"interpretations of the entered data.\n"
};

#ifdef TEST_PARSERS

/* ***************************** *
 * * TEST * TEST * TEST * TEST * *
 * ***************************** */
int
main (void)
{

	/* MAKE4 - used for verification... */
#define MAKE4(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

	/* Some very, and i do mean VERY basic verification... */

	printf ("[testing] Testing all functions\n");
	/* Decimal */
	{
		char in_data[] = "1234";
		int index = 0;
		parse_data_t data = {parse_dec, 0, 1, 1, "Decimal"};
		
		printf ("[test] Decimal... ");

		for (index = sizeof(in_data) - 2; index >= 0; index--)
		{
			data.parse(in_data[index], &data);
		}
		if (data.value != 1234)
			return 1;
		printf ("[done]\n");
	}

	/* Hexadecimal */
	{
		char in_data[] = "0xf00f";
		int index = 0;
		parse_data_t data = {parse_hex, 0, 0, 1, "Hexadecimal"};

		printf ("[test] Hexadecimal... ");

		for (index = sizeof(in_data) - 2; index >= 0; index--)
		{
			data.parse(in_data[index], &data);
		}
		if (data.value != 0xf00f)
			return 1;
		printf ("[done]\n");
	}

	/* Character */
	{
		char in_data[] = "PING";
		int index = 0;
		parse_data_t data = {parse_chr, 0, 0, 0, "Character"};

		printf ("[test] Character... ");

		for (index = sizeof(in_data) - 2; index >= 0; index--)
		{
			data.parse(in_data[index], &data);
		}
		if (data.value != MAKE4('P', 'I', 'N', 'G'))
			return 1;
		printf ("[done]\n");
	}

	/* Binary */
	{
		char in_data[] = "1101\0";
		int index = 0;
		parse_data_t data = {parse_bin, 0, 0, 1, "Binary"};

		printf ("[test] Binary... ");

		for (index = sizeof(in_data) - 2; index >= 0; index--)
		{
			data.parse(in_data[index], &data);
		}
		if (data.value != 13)
			return 1;
		printf ("[done]\n");
	}
	(void)parse_oct;

#undef MAKE4

	return 0;
}
/* ***************************** *
 * * TEST * TEST * TEST * TEST * *
 * ***************************** */

#else /* TEST_PARSERS */

int
main (int argc, char **argv)
{
	unsigned int index = 0, is_negative = 0, match_all = 0, has_displayed = 0;
	int negative = 0, endian = 0;
	char *last = NULL, *first = NULL;

	parse_data_t data[] = {
		{parse_dec, 0, 1, 1, "Decimal"},	/* The offset value has to be 1 here or all mults will fail! */
		{parse_hex, 0, 0, 1, "Hexadecimal"},
		{parse_chr, 0, 0, 0, "Character"},	/* Wants the negation sign as a actual character */
		{parse_bin, 0, 0, 1, "Binary"},
		{parse_oct, 0, 0, 1, "Octal"},	/* Anyone that actually uses this? */
	};

	/* No options, no input data */
	if (argc < 2)
	{
		printf (usage_information);
		return 1;
	}

	last = first = argv[argc - 1];

	/* Handle options */
	if (argc > 2)
	{
		argc--; /* Avoid the acutual data */

		for (index = 0; (signed int)index < argc; index++)
		{
			char *tmp = argv[index];
			while (*tmp)
			{
				switch (*tmp)
				{
					case '-':
						break;
					case 'l':
						if (*(tmp + 1) == 'e')
						{
							endian = ENDIAN_LITTLE;
							tmp++;
						}
						break;
					case 'b':
						if (*(tmp + 1) == 'e')
						{
							endian = ENDIAN_BIG;
							tmp++;
						}
						break;
					case 'a':
						match_all = 1;
						break;
				}
				tmp++;
			}
		}
	}

	/* Find the terminating zero */
	for (; *last; last++); 
	last--;

	/* Find any negation marks */
	for (; *(first + negative) == '-'; negative++);

	/* Is there a uneven amount? */
	if (negative % 2)
		is_negative++;

	/* Parse the data */
	for (; first <= last; last--)
	{
		register char p = *(char *)last;
		for (index = 0; index < (sizeof(data)/sizeof(parse_data_t)); index++)
		{
			if (data[index].parse != 0)
			{
				/* If this value can have negative values then we don't want to 
				 * actually parse the negation marks */
				if (data[index].handle_neg && (first + negative) > last)
					continue;
				if (data[index].parse(p, &data[index]) == 0)
					data[index].parse = 0;
			}
		}
	}

	/* Did we actually parse any data? */
	{
		int error_count = 0;

		for (index = 0; index < sizeof(data)/sizeof(parse_data_t); index++)
		{
			if (data[index].parse == 0)
				error_count++;
		}

		if (error_count == sizeof(data)/sizeof(parse_data_t))
		{
			printf (usage_information);
			return 1;
		}
	}

	for (index = 0; index < (sizeof(data)/sizeof(parse_data_t)); index++)
	{
		unsigned int no_bytes = 1, high = 0;

		/* If parse is 0, then we know that the parse failed,
		 * so we'll skip this part all together... */
		if (data[index].parse == 0)
			continue;
		if (has_displayed++ && match_all == 0)
			break;

		/* How many bytes of data do we have? */
		{
			uint64_t value = 0;

			/* 64 bit or 32 bit? */
			if ((value = GET_HI(data[index].value)))
				high = 1;
			else
				value = GET_LO(data[index].value);

			/* Binary search for the highest byte with a value */
			if (value & 0xffff0000)
			{
				if ((value & 0xff000000) == 0)
					no_bytes = 3;
				else
					no_bytes = 4;
			}
			else if (value & 0x0000ff00)
				no_bytes = 2;

			if (high)
				no_bytes += 4;
		}

		/* Change endianess? */
		switch (endian)
		{
			case ENDIAN_LITTLE:
			{
				unsigned char *bytes = (unsigned char *)&data[index].value;
				uint64_t value = 0;
				int offset = -16; /* Initial bit shift should be zero, tus we have to set a negative value here */

				/* We need a even value */
				if (no_bytes % 2 != 0)
					no_bytes++;

				/* Since we have a even value we can process two bytes at a time. */
				switch (no_bytes)
				{
					case 8: 	value |= 	(((uint64_t)bytes[7] | ((uint64_t)bytes[6] << 8)) << (offset += 16));
					case 6:	value |= 	(((uint64_t)bytes[5] | ((uint64_t)bytes[4] << 8)) << (offset += 16));
					case 4:	value |= 	(((uint64_t)bytes[3] | ((uint64_t)bytes[2] << 8)) << (offset += 16));
					case 2:	value |=	(((uint64_t)bytes[1] | ((uint64_t)bytes[0] << 8)) << (offset += 16));
				}
				data[index].value = value;
				break;
			}
			default:
				break;
		}

		/* Negate the value if needed, we also limit the size of the new value
		 * so we don't get 64bit negative values for a common value */
		if (is_negative && data[index].handle_neg)
		{
			data[index].value *= (-1);
			data[index].value &= ((uint64_t)(-1) >> (64 - (no_bytes << 3)));
		}

		printf ("When input is parsed as: %s\nNumber of bits used: %i (%i bytes)\n", 
			data[index].name, no_bytes << 3, no_bytes);

		/* Decimal */
		printf ("Dec: %-22"PRIu64, data[index].value);
		if (high)
			printf (" --- hi: %-11"PRIu64" lo: %"PRIu64, GET_HI(data[index].value), GET_LO(data[index].value));
		printf ("\n");

		/* Hexadecimal */
		printf ("Hex: 0x%-20"PRIx64, data[index].value);
		if (high)
			printf (" --- hi: 0x%-9"PRIx64" lo: 0x%"PRIx64, GET_HI(data[index].value), GET_LO(data[index].value));
		printf ("\n");

		/* Octal */
		printf ("Oct: %-22"PRIo64, data[index].value);
		if (high)
			printf (" --- hi: %-11"PRIo64" lo: %"PRIo64, GET_HI(data[index].value), GET_LO(data[index].value));
		printf ("\n");

		/* Ascii */
		{
			char *tmp = NULL, buffer[65] = {0};
			int idx = 0;

			for (tmp = buffer, idx = (no_bytes -1) << 3; idx > -1; idx -= 8, tmp++)
			{
				if (!isprint(*tmp = (((((uint64_t)0xff << idx)) & data[index].value) >> idx)))
					*tmp = '.';
				if (idx == 32)
				{
					*(++tmp) = ' ';
					*(++tmp) = '|';
					*(++tmp) = ' ';
				}
			}
			printf ("Asc: %s\n", buffer);
		}

		/* Binary */
		{
			char *tmp = NULL, buffer[70] = {0};
			int idx = no_bytes << 3;
			uint64_t bit = (uint64_t)1 << (idx - 1);

			for (tmp = buffer, idx = 64 - idx; idx < 64; idx++)
			{
				*tmp++ = (data[index].value & bit) ? '1' : '0';
				bit >>= 1;

				if (idx == 31)
				{
					*tmp++ = ' ';
					*tmp++ = '|';
					*tmp++ = ' ';
				}
			}
			printf ("Bin: %s\n", buffer);
		}
		
		/* Ruler */
		{
			int nibble = 0;

			printf ("     ");
			for (nibble = no_bytes; nibble; nibble--)
			{
				printf ("^---+---");
				
				if (nibble == 5)
					printf ("   ");
			}
			printf ("\n\n");
		}
	}
	return 0;
}
#endif /* TESTED_PARSERS */

