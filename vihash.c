/*
 * vihash
 *
 * Portions Copyright (c) 2010, Peter Eisentraut
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 */

#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;


PG_FUNCTION_INFO_V1(vihashtext);

Datum vihashtext(PG_FUNCTION_ARGS);
Datum vihash_any(register const unsigned char *k, register int keylen);


Datum
vihashtext(PG_FUNCTION_ARGS)
{
	text	   *key = PG_GETARG_TEXT_PP(0);
	Datum		result;

	/*
	 * Note: this is currently identical in behavior to hashvarlena, but keep
	 * it as a separate function in case we someday want to do something
	 * different in non-C locales.	(See also hashbpchar, if so.)
	 */
	result = vihash_any((unsigned char *) VARDATA_ANY(key),
					  VARSIZE_ANY_EXHDR(key));

	/* Avoid leaking memory for toasted inputs */
	PG_FREE_IF_COPY(key, 0);

	return result;
}

/*
 * This hash function was written by Bob Jenkins
 * (bob_jenkins@burtleburtle.net), and superficially adapted
 * for PostgreSQL by Neil Conway. For more information on this
 * hash function, see http://burtleburtle.net/bob/hash/doobs.html,
 * or Bob's article in Dr. Dobb's Journal, Sept. 1997.
 */

/*----------
 * mix -- mix 3 32-bit values reversibly.
 * For every delta with one or two bits set, and the deltas of all three
 * high bits or all three low bits, whether the original value of a,b,c
 * is almost all zero or is uniformly distributed,
 * - If mix() is run forward or backward, at least 32 bits in a,b,c
 *	 have at least 1/4 probability of changing.
 * - If mix() is run forward, every bit of c will change between 1/3 and
 *	 2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
 *----------
 */
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= ((c)>>13); \
  b -= c; b -= a; b ^= ((a)<<8); \
  c -= a; c -= b; c ^= ((b)>>13); \
  a -= b; a -= c; a ^= ((c)>>12);  \
  b -= c; b -= a; b ^= ((a)<<16); \
  c -= a; c -= b; c ^= ((b)>>5); \
  a -= b; a -= c; a ^= ((c)>>3);	\
  b -= c; b -= a; b ^= ((a)<<10); \
  c -= a; c -= b; c ^= ((b)>>15); \
}

/*
 * hash_any() -- hash a variable-length key into a 32-bit value
 *		k		: the key (the unaligned variable-length array of bytes)
 *		len		: the length of the key, counting by bytes
 *
 * Returns a uint32 value.	Every bit of the key affects every bit of
 * the return value.  Every 1-bit and 2-bit delta achieves avalanche.
 * About 6*len+35 instructions. The best hash table sizes are powers
 * of 2.  There is no need to do mod a prime (mod is sooo slow!).
 * If you need less than 32 bits, use a bitmask.
 */
Datum
vihash_any(register const unsigned char *k, register int keylen)
{
	register uint32 a,
				b,
				c,
				len;

	/* Set up the internal state */
	len = keylen;
	a = b = 0x9e3779b9;			/* the golden ratio; an arbitrary value */
	c = 3923095;				/* initialize with an arbitrary value */

	/* handle most of the key */
	while (len >= 12)
	{
		a += (k[0] + ((uint32) k[1] << 8) + ((uint32) k[2] << 16) + ((uint32) k[3] << 24));
		b += (k[4] + ((uint32) k[5] << 8) + ((uint32) k[6] << 16) + ((uint32) k[7] << 24));
		c += (k[8] + ((uint32) k[9] << 8) + ((uint32) k[10] << 16) + ((uint32) k[11] << 24));
		mix(a, b, c);
		k += 12;
		len -= 12;
	}

	/* handle the last 11 bytes */
	c += keylen;
	switch (len)				/* all the case statements fall through */
	{
		case 11:
			c += ((uint32) k[10] << 24);
		case 10:
			c += ((uint32) k[9] << 16);
		case 9:
			c += ((uint32) k[8] << 8);
			/* the first byte of c is reserved for the length */
		case 8:
			b += ((uint32) k[7] << 24);
		case 7:
			b += ((uint32) k[6] << 16);
		case 6:
			b += ((uint32) k[5] << 8);
		case 5:
			b += k[4];
		case 4:
			a += ((uint32) k[3] << 24);
		case 3:
			a += ((uint32) k[2] << 16);
		case 2:
			a += ((uint32) k[1] << 8);
		case 1:
			a += k[0];
			/* case 0: nothing left to add */
	}
	mix(a, b, c);

	/* report the result */
	return UInt32GetDatum(c);
}
