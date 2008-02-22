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
typedef signed   long long int64_t;
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
typedef   signed __int64 int64_t;

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
	ENDIAN_BIG,		/**< for big endian, default */
	ENDIAN_LITTLE		/**< for little endian */
} endian_t;

struct _parse_data_t;

typedef int (*parse_func_t)(unsigned char p, struct _parse_data_t *data);

typedef struct _parse_data_t
{
	parse_func_t parse;		/**< a pointer to a parser function */
	uint64_t value;		/**< where we save the parsed values */
	int64_t offset; 		/**< here we store any kind of additional state we need while parsing */
	int is_negative;		/**< here we store if we have read a negative value or not */
	char name[15];			/**< the name of the type of value we are parsing */
} parse_data_t;

static int parse_dec (unsigned char p, parse_data_t *data);
static int parse_hex (unsigned char p, parse_data_t *data);
static int parse_chr (unsigned char p, parse_data_t *data);
static int parse_bin (unsigned char p, parse_data_t *data);

#if 0
#define DEBUG(x, y) printf("[DEBUG] %s name: %s value: %c\n", __FUNCTION__, (y), (x))
#else
#define DEBUG(x, y);
#endif

/* =========================================================================
 * Parse functions
 * ========================================================================= */

/** Parse a decimal string to a integer value.
 * @param p a character from the end of the input string.
 * @param data the data struct for the parser.
 * @return 1 if we can handle the calue else 0.
 */
static int
parse_dec (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);
	switch (p)
	{
		case '-':
			data->value *= -1;
			data->is_negative ^= 1;
			/*@fallthrough@*/
		case '+':
			return 1;
	}

	if (isdigit(p))
	{
		data->value += (uint64_t)(p - (unsigned char)'0') * data->offset;
		data->offset *= 10;

		return 1;
	}
	else
		return 0;
}

/** Parse a hexadecimal string to a integer value.
 * @param p a character from the end of the input string.
 * @param data the data struct for the parser.
 * @return 1 if we can handle the value else 0.
 */
static int
parse_hex (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);

	/* Hex can have a 0x prefix */
	switch (p)
	{
		case '-':
			data->value *= -1;
			data->is_negative ^= 1;
			/*@fallthrough@*/
		case '+':
		case 'x':
			data->offset = ~0;
			return 1;
	}

	if (data->offset == ~0)
		return 1;

	if (isalnum(p) && toupper(p) < (int)'G')
	{
		data->value += (uint64_t)(isdigit(p) ? (int)(p - (unsigned char)'0') : 
			(toupper(p) - ((int)'A' - 10))) << (uint64_t)data->offset;
		data->offset += 4;

		return 1;
	}
	else
		return 0;
}

/** Parse a character string to a integer value.
 * @param p a character from the end of the input string.
 * @param data the data struct for the parser.
 * @return 1 if we can handle the value else 0.
 */
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
		data->value += ((uint64_t)p) << (uint64_t)data->offset;
		data->offset += 8;
	}
	return 1;
}

/** Parse a binary string (1 or 0) to a integer value.
 * @param p a character from the end of the input string.
 * @param data the data struct for the parser.
 * @return 1 if we can handle the value else 0.
 */
static int
parse_bin (unsigned char p, parse_data_t *data)
{
	DEBUG(p, data->name);

	switch (p)
	{
		case '0':
		case '1':
			data->value += (uint64_t)(p - (unsigned char)'0') << (uint64_t)data->offset;
			data->offset += 1;
			/*@fallthrough@*/
		case '%': /* Binary data can be prefixed with % */
			return 1;
		case '-':
			data->value *= -1;
			data->is_negative ^= 1;
			return 1;
		default:
			return 0;
	}
}

/* =========================================================================
 * Help text
 * ========================================================================= */

static const char usage_information[] = {
	"shownum " VERSION " - http://zapek.com/?page_id=17\n"
	"            | http://pomac.netswarm.net/misc/\n\n"
	"usage: shownum <val>\n"
	"where val is one of:\n"
	" - decimal (eg. 5)\n"
	" - hexadecimal (eg. 0x5)\n"
	" - ascii chars, 8 max (eg. foo)\n"
	" - binary, 64-bits max (eg. 1101)\n"
	"negative numbers work too\n"
	"\noptions:\n"
	" -a -- display all matches\n"
	"\nendianess:\n"
	" -le -- little endian\n"
	" -be -- big endian (default)\n"
};

#ifdef TEST_PARSERS

/* ***************************** *
 * * TEST * TEST * TEST * TEST * *
 * ***************************** */

typedef struct {
	char *string;
	uint64_t value;
} test_values_t;

typedef struct {
	parse_data_t data;
	test_values_t values[3];
} test_data_t;

int
main (void)
{

	/* MAKE4 - used for verification... */
#define MAKE4(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

	test_data_t test_data[] =
	{
		{
			{ parse_dec, 0, 1, 0, "Decimal" },
			{
				{ "1234", 1234 }, 
				{ "4294967295", 4294967295 }, 
				{ "18446744073709551615", 0xffffffffffffffff },
			},
		},
		{
			{ parse_hex, 0, 0, 0, "Hexadecimal" },
			{
				{ "0xf00f", 0xf00f },
				{ "0xffffffff", 0xffffffff },
				{ "0xffffffffffffffff", 0xffffffffffffffff },
			},
		},
		{
			{ parse_bin, 0, 0, 0, "Binary" },
			{
				{ "1111111111111111", 0xffff },
				{ "11111111111111111111111111111111", 0xffffffff },
				{ "1111111111111111111111111111111111111111111111111111111111111111", 0xffffffffffffffff },
			},
		},
		{
			{ parse_chr, 0, 0, 0, "Character" },
			{
				{ "FOO", MAKE4('\0', 'F', 'O', 'O') },
				{ "PING", MAKE4('P', 'I', 'N', 'G') },
				{ "PINGPONG", (uint64_t)MAKE4('P', 'I', 'N', 'G') << 32 | MAKE4('P', 'O', 'N', 'G') },
			},
		},
	};

	char *first = NULL, *last = NULL;
	unsigned int entry = 0, top_level = 0;

	for (; top_level < sizeof(test_data)/sizeof(test_data_t); top_level++)
	{
		printf ("\n");

		for (entry = 0; entry < sizeof(test_data[top_level].values)/sizeof(test_data[top_level].values[0]); entry++)
		{
			parse_data_t data = test_data[top_level].data;

			first = last = test_data[top_level].values[entry].string;
			for (; *last != '\0'; last++);
			last--;

			while (last >= first)
				data.parse(*last--, &data);

			if (data.value != test_data[top_level].values[entry].value)
			{
				printf("Error comparing %"PRIu64" to %"PRIu64"\n", data.value, test_data[top_level].values[entry].value);
				return 1;
			}

			printf ("\033[A  * (%u/%u) %s...\n", entry + 1, 
				(unsigned int)(sizeof(test_data[top_level].values)/sizeof(test_data[top_level].values[0])), data.name);
		}
	}

#undef MAKE4

	return 0;
}
/* ***************************** *
 * * TEST * TEST * TEST * TEST * *
 * ***************************** */

#else /* TEST_PARSERS */

/* =========================================================================
 * Helper functions
 * ========================================================================= */

/** Get the number of bytes the value occupies.
 *
 * This is used so we can minimize the size of the value we output.
 *
 * @param data the parser data.
 * @return the number of bytes this value occupies.
 */
static unsigned int
get_no_bytes(const parse_data_t * const data)
{
	uint64_t value = data->is_negative ? data->value * -1 : data->value;
	unsigned int no_bytes = 1, high = 0;

	/* 64 bit or 32 bit? */
	if (GET_HI(value))
	{
		value = GET_HI(value);
		high = 1;
	}

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
	
	return no_bytes;
}

/** Switch the endian of a value.
 *
 * This function calls get_no_bytes and sometimes has to add a byte for the 
 * endian switcheroo, thus it returns the number of bytes the value occupies.
 *
 * It modifies the value in the data struct.
 *
 * @param data the parser data.
 * @param endian what endian it should be in.
 * @return the number of bytes this value occupies.
 */
static unsigned int
switch_endian(parse_data_t *data, endian_t endian)
{
	unsigned int no_bytes = get_no_bytes(data);

	switch (endian)
	{
		case ENDIAN_LITTLE:
		{
			unsigned char *bytes = (unsigned char *)&data->value;
			uint64_t value = 0;
			int offset = -16; /* Initial bit shift should be zero, tus we have to set a negative value here */

			/* We need a even value */
			if (no_bytes % 2 != 0)
				no_bytes++;

			/* Since we have a even value we can process two bytes at a time. */
			switch (no_bytes)
			{
				case 8:
					value |= (((uint64_t)bytes[7] | ((uint64_t)bytes[6] << 8)) << (unsigned int)(offset += 16));
					/*@fallthrough@*/
				case 6:
					value |= (((uint64_t)bytes[5] | ((uint64_t)bytes[4] << 8)) << (unsigned int)(offset += 16));
					/*@fallthrough@*/
				case 4:
					value |= (((uint64_t)bytes[3] | ((uint64_t)bytes[2] << 8)) << (unsigned int)(offset += 16));
					/*@fallthrough@*/
				case 2:
					value |= (((uint64_t)bytes[1] | ((uint64_t)bytes[0] << 8)) << (unsigned int)(offset += 16));
			}
			data->value = value;
			break;
		}
		default:
			break;
	}
	return no_bytes;
}

/* =========================================================================
 * Output functions
 * ========================================================================= */

/** Print the ascii values if any.
 *
 * Prints all "printable" characters in a value.
 *
 * @param data the parse data.
 * @param no_bytes the number of bytes we should output.
 */
static void
print_ascii(const parse_data_t * const data, const unsigned int no_bytes)
{
	char *tmp = NULL, buffer[65] = {'\0'};
	int idx = 0;

	for (tmp = buffer, idx = (int)((no_bytes -1) << 3); idx > -1; idx -= 8, tmp++)
	{
		if (!isprint(*tmp = (char)(((((uint64_t)0xff << (unsigned int)idx)) & data->value) >> (unsigned int)idx)))
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

/** Print the value in a binary representation.
 *
 * Basically prints the value as ones and zeros.
 *
 * @param data the parse data.
 * @param no_bytes the number of bytes we should output.
 */
static void
print_binary(const parse_data_t * const data, const unsigned int no_bytes)
{
	char *tmp = NULL, buffer[70] = {'\0'};
	unsigned int idx = no_bytes << 3;
	uint64_t bit = (uint64_t)1 << (unsigned int)(idx - 1);

	for (tmp = buffer, idx = 64 - idx; idx < 64; idx++)
	{
		*tmp++ = (data->value & bit) ? '1' : '0';
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

/** Print the ruler.
 *
 * The ruler shows the nibble and the byte borders of the binary representation.
 * '^' is a byte separator while '+' is a nibble separator.
 *
 * @param no_bytes the number of bytes we should show.
 */
static void
print_ruler(const unsigned int no_bytes)
{
	int nibble = 0;

	printf ("     ");
	for (nibble = (int)no_bytes; nibble; nibble--)
	{
		printf ("^---+---");
		if (nibble == 5)
			printf ("   ");
	}
	printf ("\n\n");
}

/* =========================================================================
 * Where it all comes together.
 * ========================================================================= */

int
main (int argc, char **argv)
{
	unsigned int index = 0, match_all = 0, has_displayed = 0, endian = 0;
	char *last = NULL, *first = NULL;

	parse_data_t data[] = {
		{parse_dec, 0, 1, 0, "Decimal"},		/* set up the decimal parser, it needs a start offset of 1 */
		{parse_hex, 0, 0, 0, "Hexadecimal"},	/* set up the hexadecimal parser */
		{parse_bin, 0, 0, 0, "Binary"},		/* set up the binary parser      */
		{parse_chr, 0, 0, 0, "Character"},		/* set up the character parser   */
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
			while ((unsigned int)*tmp)
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
	for (; (unsigned int)*last; last++); 
	last--;

	/* Parse the data */
	for (; first <= last; last--)
	{
		register unsigned char p = *(unsigned char *)last;
		for (index = 0; index < (sizeof(data)/sizeof(parse_data_t)); index++)
		{
			if (data[index].parse != NULL)
			{
				if (data[index].parse(p, &data[index]) == 0)
					data[index].parse = NULL;
			}
		}
	}

	/* Did we actually parse any data? */
	{
		int error_count = 0;

		for (index = 0; index < sizeof(data)/sizeof(parse_data_t); index++)
		{
			if (data[index].parse == NULL)
				error_count++;
		}

		if (error_count == sizeof(data)/sizeof(parse_data_t))
		{
			printf (usage_information);
			return 1;
		}
	}

	/* Print the output */
	for (index = 0; index < (sizeof(data)/sizeof(parse_data_t)); index++)
	{
		unsigned int no_bytes = 0, high = 0;

		/* If parse is 0, then we know that the parse failed,
		 * so we'll skip this part all together... */
		if (data[index].parse == NULL)
			continue;

		/* If we have displayed a entry and we don't want to see
		 * all matches, then stop */
		if (has_displayed++ > 0 && match_all == 0)
			break;

		/* Find out how many bytes we have and switch endian */
		no_bytes = switch_endian(&data[index], endian);

		/* Trim the value down to the bytes needed */
		if (data[index].is_negative > 0)
			data[index].value &= ((uint64_t)(-1) >> (64 - (no_bytes << 3)));

		high = GET_HI(data[index].value);

		printf ("When input is parsed as: %s\nNumber of bits used: %u (%u bytes)\n", 
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
		print_ascii(&data[index], no_bytes);

		/* Binary */
		print_binary(&data[index], no_bytes);
	
		/* Ruler */
		print_ruler(no_bytes);
	}
	return 0;
}
#endif /* TESTED_PARSERS */

