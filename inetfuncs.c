#include <stdio.h>
#include <inttypes.h>
#include "inetfuncs.h"

int ip_pton(
	const unsigned char *zIn,
	sqlite3_int64 *nOut
){
	unsigned a, b, c, d;
	sqlite3_int64 r;
	int rc;
	rc = sscanf(zIn, "%u.%u.%u.%u", &a, &b, &c, &d);
	if( rc != 4 ) return -2;
	//printf("%u,%u,%u,%u\n", a, b, c, d);
	if( a > 255 || b > 255 || c > 255 || d > 255 ) return -1;
	r = (a << 24) + (b << 16) + (c << 8) + d;
	*nOut = r;
	return 0;
}

void ip_pton_func(
	sqlite3_context *ctx,
	int argc,
	sqlite3_value **argv
){
	int rc;
	const unsigned char *zIn;
	sqlite3_int64 nOut;
	if( sqlite3_value_type(argv[0])!=SQLITE_TEXT ) return;
	zIn = (const unsigned char *)sqlite3_value_text(argv[0]);
	rc = ip_pton(zIn, &nOut);
	switch(rc) {
	case -1:
		//sqlite3_result_error(ctx,"out of range", -1);
		sqlite3_result_null(ctx);
		return;
	case -2:
		//sqlite3_result_error(ctx,"error in sscanf", -1);
		sqlite3_result_null(ctx);
		return;
	}
	sqlite3_result_int64(ctx, nOut);
}

int ip_ntop(
	long unsigned nIn,
	char *zOut,
	size_t zOut_size
){
	unsigned int a, b, c, d;
	a = (nIn & 0xff000000) >> 24;
	b = (nIn & 0x00ff0000) >> 16;
	c = (nIn & 0x0000ff00) >> 8;
	d = (nIn & 0x000000ff) >> 0;
	int rc = snprintf(zOut, zOut_size, "%u.%u.%u.%u", a, b, c, d);
	return rc;
}

void ip_ntop_func(
	sqlite3_context *ctx,
	int argc,
	sqlite3_value **argv
){
	int rc;
	sqlite3_int64 nIn;
	char zOut[16];
	if (sqlite3_value_numeric_type(argv[0]) != SQLITE_INTEGER) return;


	nIn = sqlite3_value_int64(argv[0]);
	rc = ip_ntop(nIn, zOut, sizeof(zOut));
	sqlite3_result_text(ctx, zOut, -1, SQLITE_TRANSIENT);
}

void ip_mask_func(
	sqlite3_context *ctx,
	int argc,
	sqlite3_value **argv
){
	int rc, maskbits, input_type;
	sqlite3_int64 ip;
	uint32_t ip32, mask;
	char zOut[16];

	input_type = sqlite3_value_numeric_type(argv[0]);
	switch (input_type) {
	case SQLITE_INTEGER:
		ip = sqlite3_value_int64(argv[0]);
		break;
	case SQLITE3_TEXT:
		rc = ip_pton(sqlite3_value_text(argv[0]), &ip);
		if (rc != 0) return;
		break;
	default:
		return;
		break;
	}
	maskbits = sqlite3_value_int(argv[1]);
	if (maskbits < 0 || maskbits > 32) return;

	ip32 = ip;
	mask = ~0 << (32-maskbits);

	ip = ip32 & mask;
	
	// choose output type to match input type
	switch (input_type) {
	case SQLITE_INTEGER:
		sqlite3_result_int(ctx, ip);
		break;
	case SQLITE3_TEXT:
		rc = ip_ntop(ip, zOut, sizeof(zOut));
		sqlite3_result_text(ctx, zOut, -1, SQLITE_TRANSIENT);
		break;
	}
}

int inetfuncs_init(
	sqlite3 *db
){
	int rc = SQLITE_OK;
	int eTextRep = SQLITE_UTF8 | SQLITE_DETERMINISTIC;
#ifdef SQLITE_DIRECTONLY
	eTextRep |= SQLITE_DIRECTONLY;
#endif

	rc = sqlite3_create_function(
	/* *db */		db,
	/* *zFunctionName */	"inet_pton",
	/* nArg */		1,
	/* eTextRep */		eTextRep,
	/* *pApp */		0,
	/* xFunc */		ip_pton_func,
	/* xStep */		0,
	/* xFinal */		0
	);
	rc = sqlite3_create_function(
	/* *db */		db,
	/* *zFunctionName */	"inet_ntop",
	/* nArg */		1,
	/* eTextRep */		eTextRep,
	/* *pApp */		0,
	/* xFunc */		ip_ntop_func,
	/* xStep */		0,
	/* xFinal */		0
	);
	rc = sqlite3_create_function(
	/* *db */		db,
	/* *zFunctionName */	"inet_mask",
	/* nArg */		2,
	/* eTextRep */		eTextRep,
	/* *pApp */		0,
	/* xFunc */		ip_mask_func,
	/* xStep */		0,
	/* xFinal */		0
	);
	return rc;
}
