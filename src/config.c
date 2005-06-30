/**
 * $Id: config.c,v 1.5 2005/05/14 09:52:02 mr-russ Exp $
 *
 * configfile parser
 *
 * Copyright (c) 2001 by Joerg Wendland, Bret Mogilefsky
 * Copyright (c) 2004,2005 by Russell Smith
 * see included file COPYING for details
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nss-pgsql.h"

#define HASHMAX 73
#define CFGLINEMAX 512

static char *_options[HASHMAX];
static unsigned int _confisopen = 0;

unsigned int texthash(const char *str);


/*
 * create a simple hash from a string
 */
unsigned int texthash(const char *str)
{
	int i, s;

	for(i = s = 0; str[i]; i++) {
		s += str[i];
	}

	return s % HASHMAX;
}

/*
 * read configfile and save values in hashtable
 */
int readconfig(char* configfile)
{
	FILE *cf;
	char line[CFGLINEMAX], key[CFGLINEMAX], val[CFGLINEMAX], *c;
	unsigned int h;
	unsigned int lineno = 0;

	if(_confisopen) {
		for(h = 0; h < HASHMAX; h++) {
			free(_options[h]);
		}
	}

	if(!(cf = fopen(configfile, "r"))) {
		DebugPrint("could not open config file  %s\n" _C_ configfile);
		return 0;
	}
	h = 0;
	while(h < HASHMAX) {
		_options[h] = NULL;
		++h;
	}

	while(fgets(line, CFGLINEMAX, cf)) {
		lineno++;

		/* remove comments */
		c = strstr(line, "#");
		if(c) {
			line[c-line] = 0;
		}

		if (*line == 0 || *line == '\n')
			continue;

		/* read options */
		if(sscanf(line, " %s = %[^\n]", key, val) < 2) {
			print_err("line %d in %s is unparseable: \"%s\"\n", lineno, configfile, line);
		} else {
			h = texthash(key);
			if (_options[h] != NULL ) {
				print_err("line %d in %s is a duplicate hash: \"%s\"\n", lineno, configfile, key);
			} else {
				_options[h] = malloc(strlen(val)+1);
				strcpy(_options[h], val);
			}
		}
	}
	fclose(cf);

	_confisopen = 1;
	atexit(cleanup);

	return 1;
}

/*
 * free the hashmap, close connection to db if open
 */
void cleanup(void)
{
	int i;

	if(_confisopen) {
		for(i = 0; i < HASHMAX; i++) {
			free(_options[i]);
		}
	}
	_confisopen = 0;

	while(backend_isopen(CONNECTION_ANY)) {
		backend_close();
	}
}


/*
 * get value for 'key' from the hashmap
 */
inline char *getcfg(const char *key)
{
	return _options[texthash(key)] ? _options[texthash(key)] : "";
}
