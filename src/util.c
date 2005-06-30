/**
 * $Id: util.c,v 1.2 2004/12/01 06:29:47 mr-russ Exp $
 *
 * public interface to libc
 *
 * Copyright (c) 2001 by Joerg Wendland, Bret Mogilefsky
 * see included file COPYING for details
 *
 */

#include "nss-pgsql.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


void print_err(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);

	fflush(stderr);
	exit(1);
}

void print_msg(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);

	fflush(stderr);
}
