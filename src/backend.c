/**
 * $Id: backend.c,v 1.12 2006/01/09 22:33:07 mr-russ Exp $
 *
 * database backend functions
 *
 * Copyright (c) 2001 by Joerg Wendland, Bret Mogilefsky
 * Copyright (c) 2004,2005 by Russell Smith
 *
 * see included file COPYING for details
 *
 */

#include "nss-pgsql.h"
#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* define group column names */
#define GROUP_NAME   0
#define GROUP_PASSWD 1
#define GROUP_GID    2
/* define passwd column names */
#define PASSWD_NAME   0
#define PASSWD_PASSWD 1
#define PASSWD_GECOS  2
#define PASSWD_DIR    3
#define PASSWD_SHELL  4
#define PASSWD_UID    5
#define PASSWD_GID    6
/* define shadow column names */
#define SHADOW_NAME   0
#define SHADOW_PASSWD 1
#define SHADOW_LSTCHG 2
#define SHADOW_MIN    3
#define SHADOW_MAX    4
#define SHADOW_WARN   5
#define SHADOW_INACT  6
#define SHADOW_EXPIRE 7
#define SHADOW_FLAG   8

static PGconn *_conn = NULL;
static PGconn *_shadowconn = NULL;
static int _isopen = 0;
static int _shadowisopen = 0;
static int _transaction = 0;
static int _shadowtransaction = 0;

int backend_isopen(char type)
{
	if (type == CONNECTION_SHADOW) {
		return (_shadowisopen > 0);
	} else if (type != CONNECTION_SHADOW) {
		return (_isopen > 0);
	} else {
		// Error Should never happen
		return -1;
	}
}

/*
 * read configuration and connect to database
 */
int backend_open(char type)
{
	if (type == CONNECTION_SHADOW) {
		if (!_shadowisopen) {
			if (readconfig(CONNECTION_SHADOW, CFGROOTFILE)) {
				if (_shadowconn != NULL) {
					PQfinish(_shadowconn);
				}
				_shadowconn = PQconnectdb(getcfg("shadowconnectionstring"));
			}

			if(PQstatus(_shadowconn) == CONNECTION_OK) {
				++_shadowisopen;
			} else {
				print_msg("\nCould not connect to database\n");
			}
		}
		return (_shadowisopen > 0);
	} else {
		if(!_isopen) {
			if (readconfig(CONNECTION_USERGROUP, CFGFILE)) {
				if (_conn != NULL) {
					PQfinish(_conn);
				}
				_conn = PQconnectdb(getcfg("connectionstring"));
			}

			if(PQstatus(_conn) == CONNECTION_OK) {
				++_isopen;
			} else {
				print_msg("\nCould not connect to database\n");
			}
		}
		return (_isopen > 0);
	}

	// Return 0, code path should never execute but stop gcc complaining
	return 0;
	
}

/*
 * close connection to database and clean up configuration
 */
void backend_close(char type)
{
	if (type == CONNECTION_SHADOW) {
		--_shadowisopen;
		if (!_shadowisopen) {
			PQfinish(_shadowconn);
			_shadowconn = NULL;
		}
		if(_shadowisopen < 0) {
			_shadowisopen = 0;
		}
	} else {
		--_isopen;
		if(!_isopen) {
			PQfinish(_conn);
			_conn = NULL;
		}
		if(_isopen < 0) {
			_isopen = 0;
		}
	}
}

/*
 *  prepare a cursor in database
 */
inline enum nss_status getent_prepare(const char *what)
{
	char *stmt;
	PGresult *res;
	ExecStatusType status;

	asprintf(&stmt, "DECLARE nss_pgsql_internal_%s_curs SCROLL CURSOR FOR "
	                "%s FOR READ ONLY", what, getcfg(what));

	if (strncmp("shadow", what, 6) == 0) {
		if (!_shadowtransaction++) {
			PQclear(PQexec(_shadowconn, "BEGIN TRANSACTION"));
		}
		res = PQexec(_shadowconn, stmt);
	} else {
		if (!_transaction++) {
			PQclear(PQexec(_conn, "BEGIN TRANSACTION"));
		}
		res = PQexec(_conn, stmt);
	}

	status = PQresultStatus(res);
	free(stmt);

	if (status == PGRES_COMMAND_OK) {
		return NSS_STATUS_SUCCESS;
	} else {
		return NSS_STATUS_UNAVAIL;
	}
}

/*
 *  close the transaction used for the cursor
*/
inline void getent_close(char type)
{
	if (type == CONNECTION_SHADOW) {
		--_shadowtransaction;
		if (!_shadowtransaction) {
			PQclear(PQexec(_shadowconn, "COMMIT"));
		}
		if (!_shadowtransaction < 0) {
			_shadowtransaction = 0;
		}
	} else {
		--_transaction;
		if (!_transaction) {
			PQclear(PQexec(_conn, "COMMIT"));
		}
		if (_transaction < 0) {
			_transaction = 0;
		}
	}
}	

/*
  With apologies to nss_ldap...
  Assign a single value to *valptr from the specified row in the result
*/
enum nss_status
copy_attr_colnum(PGresult *res, int attrib_number, char **valptr,
                 char **buffer, size_t *buflen, int *errnop, int row)
{

	const char *sptr;
	size_t slen;

	sptr = PQgetvalue(res, row, attrib_number);
	slen = strlen(sptr);
	if(*buflen < slen+1) {
		*errnop = ERANGE;
		return NSS_STATUS_TRYAGAIN;
	}
	strncpy(*buffer, sptr, slen);
	(*buffer)[slen] = '\0';
		
	*valptr = *buffer;

	*buffer += slen + 1;
	*buflen -= slen + 1;

	return NSS_STATUS_SUCCESS;
}


/*
 * return array of strings containing usernames that are member of group with gid 'gid'
 */
enum nss_status getgroupmem(gid_t gid,
                            struct group *result,
                            char *buffer,
                            size_t buflen, int *errnop)
{
	char *params[1];
	PGresult *res;
	int n, t = 0;
	enum nss_status status = NSS_STATUS_NOTFOUND;
	size_t ptrsize;

	params[0] = malloc(12);
	n = snprintf(params[0], 12, "%d", gid);
	res = NULL;
	if (n == -1 || n > 12) {
		status = NSS_STATUS_UNAVAIL;
		*errnop = EAGAIN;
		goto BAIL_OUT;
	}

	res = PQexecParams(_conn, getcfg("getgroupmembersbygid"), 1, NULL, (const char**)params, NULL, NULL, 0);

	if(PQresultStatus(res) != PGRES_TUPLES_OK) {
		status = NSS_STATUS_UNAVAIL;
		goto BAIL_OUT;
	}

	n = PQntuples(res);

	// Make sure there's enough room for the array of pointers to group member names
	ptrsize = (n+1) * sizeof(const char *);
	if(buflen < ptrsize) {
		status = NSS_STATUS_TRYAGAIN;
		*errnop = ERANGE;
		goto BAIL_OUT;
	}

	/* realign the buffer on a 4-byte boundary */
	buflen -= 4-((long)buffer & 0x3);
	buffer += 4-((long)buffer & 0x3);

	result->gr_mem = (char**)buffer;

	buffer += (ptrsize+3)&(~0x3);
	buflen -= (ptrsize+3)&(~0x3);

	for(t = 0; t < n; t++) {
		// Should return only 1 column, use the first one.
		// FIXME: LOG error if issues
		status = copy_attr_colnum(res, 0, &(result->gr_mem[t]), &buffer, &buflen, errnop, t);
		if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;
	}
	// If the group has no members, there was still success
	if (n == 0) {
		*errnop = 0;
		status = NSS_STATUS_SUCCESS;
	}
	result->gr_mem[n] = NULL;
	
 BAIL_OUT:

	PQclear(res);
	free(params[0]);

	return status;
}

/*
 * 'convert' a PGresult to struct group
 */
enum nss_status res2grp(PGresult *res,
                        struct group *result,
                        char *buffer,
                        size_t buflen, int *errnop)
{
	enum nss_status status = NSS_STATUS_NOTFOUND;
#ifdef DEBUG
	char **i;
#endif

	if(!PQntuples(res)) {
		*errnop = 0;
		goto BAIL_OUT;
	}

	// Carefully copy attributes into buffer.  Return NSS_STATUS_TRYAGAIN if not enough room.
	status = copy_attr_colnum(res, GROUP_NAME, &result->gr_name, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	status = copy_attr_colnum(res, GROUP_PASSWD, &result->gr_passwd, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	result->gr_gid = (gid_t)strtoul(PQgetvalue(res, 0, GROUP_GID), (char**)NULL, 10);

	status = getgroupmem(result->gr_gid, result, buffer, buflen, errnop);

#ifdef DEBUG
	if (status == NSS_STATUS_SUCCESS) {
		DebugPrint("Converted a res to a grp:");
		DebugPrint("GID: %d, " _C_ result->gr_gid);
		DebugPrint("Name: %s, " _C_ result->gr_name);
		DebugPrint("Password: %s, " _C_ result->gr_passwd);
		DebugPrint("Member:");
		i = result->gr_mem;
		while(*i) {
			DebugPrint("%s, " _C_ *i++);
		}
		DebugPrint("\n");
	}
#endif

 BAIL_OUT:
	return status;
}

/*
 * 'convert' a PGresult to struct passwd
 */
enum nss_status res2pwd(PGresult *res, struct passwd *result,
                        char *buffer,
                        size_t buflen, int *errnop)
{
	enum nss_status status = NSS_STATUS_NOTFOUND;

	if(!PQntuples(res)) {
		goto BAIL_OUT;
	}

	// Carefully copy attributes into buffer.  Return NSS_STATUS_TRYAGAIN if not enough room.
	// Must return passwd_name, passwd_passwd, passwd_gecos, passwd_dir, passwd_shell, passwd_uid, passwd_gid
	status = copy_attr_colnum(res, PASSWD_NAME, &result->pw_name, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	status = copy_attr_colnum(res, PASSWD_PASSWD, &result->pw_passwd, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	status = copy_attr_colnum(res, PASSWD_GECOS, &result->pw_gecos, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	status = copy_attr_colnum(res, PASSWD_DIR, &result->pw_dir, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	status = copy_attr_colnum(res, PASSWD_SHELL, &result->pw_shell, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	// Can be less careful with uid/gid
	result->pw_uid = (uid_t) strtoul(PQgetvalue(res, 0, PASSWD_UID), (char**)NULL, 10);
	result->pw_gid = (gid_t) strtoul(PQgetvalue(res, 0, PASSWD_GID), (char**)NULL, 10);

	DebugPrint("Converted a res to a pwd:\n");
	DebugPrint("UID: %d\n" _C_ result->pw_uid);
	DebugPrint("GID: %d\n" _C_ result->pw_gid);
	DebugPrint("Name: %s\n" _C_ result->pw_name);
	DebugPrint("Password: %s\n" _C_ result->pw_passwd);
	DebugPrint("Gecos: %s\n" _C_ result->pw_gecos);
	DebugPrint("Dir: %s\n" _C_ result->pw_dir);
	DebugPrint("Shell: %s\n" _C_ result->pw_shell);

BAIL_OUT:
	return status;
}

/*
 * fetch a row from cursor
 */
PGresult *fetch(char *what)
{
	char *stmt;
	PGresult *res;

	asprintf(&stmt, "FETCH FROM nss_pgsql_internal_%s_curs", what);
	if (strncmp("shadow", what, 6) == 0) {
		if (_shadowconn == NULL) {
			DebugPrint("Did a putback of cursor row with the database closed!");
			return NULL;
		}
		if (PQstatus(_shadowconn) != CONNECTION_OK) {
			DebugPrint("oops! die connection is futsch");
			return NULL;
		}
		res = PQexec(_shadowconn, stmt);
	} else {
		if(_conn == NULL) {
			DebugPrint("Did a fetch with the database closed!");
			return NULL;
		}
		if(PQstatus(_conn) != CONNECTION_OK) {
			DebugPrint("oops! die connection is futsch");
			return NULL;
		}
		res = PQexec(_conn, stmt);
	}
	free(stmt);

	return res;
}

/*
 * put back on to the cursor a row, used when out of buffer space,
 * and we need to 
 */
PGresult *putback(char *what)
{
	char *stmt;
	PGresult *res;

	asprintf(&stmt, "MOVE BACKWARD 1 IN nss_pgsql_internal_%s_curs", what);
	if (strncmp("shadow", what, 6) == 0) {
		if (_shadowconn == NULL) {
			DebugPrint("Did a putback of cursor row with the database closed!");
			return NULL;
		}
		if (PQstatus(_shadowconn) != CONNECTION_OK) {
			DebugPrint("oops! die connection is futsch");
			return NULL;
		}
		res = PQexec(_shadowconn, stmt);
	} else {
		if(_conn == NULL) {
			DebugPrint("Did a putback of cursor row with the database closed!");
			return NULL;
		}
		if(PQstatus(_conn) != CONNECTION_OK) {
			DebugPrint("oops! die connection is futsch");
			return NULL;
		}
		res = PQexec(_conn, stmt);
	}
	free(stmt);
		
	return res;
}

/*
 * get a group entry from cursor
 */
enum nss_status backend_getgrent(struct group *result,
                                 char *buffer,
                                 size_t buflen,
                                 int *errnop)
{
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;

   	*errnop = 0;
	res = fetch("allgroups");
	if(PQresultStatus(res) == PGRES_TUPLES_OK) {
		status = res2grp(res, result, buffer, buflen, errnop);
	}
	PQclear(res);
	if (status == NSS_STATUS_TRYAGAIN && *errnop == ERANGE) {
		res = putback("allgroups");
		PQclear(res);
	}
	return status;
}    

/*
 * get a passwd entry from cursor
 */
enum nss_status backend_getpwent(struct passwd *result,
                                 char *buffer,
                                 size_t buflen,
                                 int *errnop)
{
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;

	res = fetch("allusers");
	if(PQresultStatus(res) == PGRES_TUPLES_OK) {
		status = res2pwd(res, result, buffer, buflen, errnop);
	}
	if (status == NSS_STATUS_TRYAGAIN && *errnop == ERANGE) {
		res = putback("allusers");
	}

	PQclear(res);
	return status;
}    

/*
 * backend for getpwnam()
 */
enum nss_status backend_getpwnam(const char *name, struct passwd *result,
                                 char *buffer, size_t buflen, int *errnop)
{
	const char *params[1];
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;

	params[0] = name;

	res = PQexecParams(_conn, getcfg("getpwnam"), 1, NULL, params, NULL, NULL, 0);
	if(PQresultStatus(res) == PGRES_TUPLES_OK) {
		// Fill result structure with data from the database
		status = res2pwd(res, result, buffer, buflen, errnop);
	}

	PQclear(res);
	return status;
}

/*
 * backend for getpwuid()
 */
enum nss_status backend_getpwuid(uid_t uid, struct passwd *result,
	char *buffer, size_t buflen, int *errnop)
{
	char *params[1];
	int n;
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;
   
   	params[0] = malloc(12);
	n = snprintf(params[0], 12, "%d", uid);
	if (n == -1 || n > 12) {
		status = NSS_STATUS_UNAVAIL;
		*errnop = EAGAIN;
	} else {
		res = PQexecParams(_conn, getcfg("getpwuid"), 1, NULL, (const char**)params, NULL, NULL, 0);

		if(PQresultStatus(res) == PGRES_TUPLES_OK) {
			// Fill result structure with data from the database
			status = res2pwd(res, result, buffer, buflen, errnop);
		}
		PQclear(res);
    }
	free(params[0]);
	return status;
}

/*
 * backend for getgrnam()
 */
enum nss_status backend_getgrnam(const char *name,
                                 struct group *result,
                                 char *buffer,
                                 size_t buflen,
                                 int *errnop)
{
	const char *params[1];
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;

   	*errnop = 0;
	params[0] = name;

	res = PQexecParams(_conn, getcfg("getgrnam"), 1, NULL, params, NULL, NULL, 0);
	if(PQresultStatus(res) == PGRES_TUPLES_OK) {
		status = res2grp(res, result, buffer, buflen, errnop);
	}
	PQclear(res);

	return status;
}


/*
 * backend for getgrgid()
 */
enum nss_status backend_getgrgid(gid_t gid,
                                 struct group *result,
                                 char *buffer,
                                 size_t buflen,
                                 int *errnop)
{
	char *params[1];
	PGresult *res;
	int n;
	enum nss_status status = NSS_STATUS_NOTFOUND;
   
   	*errnop = 0;
   	params[0] = malloc(12);
	n = snprintf(params[0], 12, "%d", gid);
	if (n == -1 || n > 12) {
		status = NSS_STATUS_UNAVAIL;
		*errnop = EAGAIN;
	} else {
		res = PQexecParams(_conn, getcfg("getgrgid"), 1, NULL, (const char**)params, NULL, NULL, 0);

		if(PQresultStatus(res) == PGRES_TUPLES_OK) {
			status = res2grp(res, result, buffer, buflen, errnop);
		}
		PQclear(res);
	}
	return status;
}


size_t backend_initgroups_dyn(const char *user,
                              gid_t group,
                              long int *start,
                              long int *size,
                              gid_t **groupsp,
                              long int limit,
                              int *errnop)
{
	char *params[2];
	PGresult *res;
	int n;
	gid_t *groups = *groupsp;
	int rows;

	params[0] = (char*)user;
	params[1] = malloc(12);
	n = snprintf(params[1], 12, "%d", group);
	if (n == -1 || n > 12) {
		return 0;
	}

	res = PQexecParams(_conn, getcfg("groups_dyn"), 2, NULL, (const char**)params, NULL, NULL, 0);

	rows = PQntuples(res);

	if(rows+(*start) > *size) {
		// Have to make the result buffer bigger
		long int newsize = rows + (*start);
		newsize = (limit > 0) ? MIN(limit, newsize) : newsize;
		*groupsp = groups = realloc(groups, newsize * sizeof(*groups));
		*size = newsize;
	}

	rows = (limit > 0) ? MIN(rows, limit - *start) : rows;

	while(rows--) {
		groups[*start] = atoi(PQgetvalue(res, rows, 0));
		*start += 1;
	}

	PQclear(res);
	free(params[1]);
    
	return *start;
}

/*
 * 'convert' a PGresult to struct shadow
 */
enum nss_status res2shadow(PGresult *res, struct spwd *result,
                           char *buffer, size_t buflen, int* errnop)
{
	enum nss_status status = NSS_STATUS_NOTFOUND;

	if(!PQntuples(res)) {
		goto BAIL_OUT;
	}

	// Carefully copy attributes into buffer.  Return NSS_STATUS_TRYAGAIN if not enough room.
	status = copy_attr_colnum(res, SHADOW_NAME, &result->sp_namp, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	status = copy_attr_colnum(res, SHADOW_PASSWD, &result->sp_pwdp, &buffer, &buflen, errnop, 0);
	if(status != NSS_STATUS_SUCCESS) goto BAIL_OUT;

	// Can be less careful with int variables
	result->sp_lstchg = (long int) atol(PQgetvalue(res, 0, SHADOW_LSTCHG));
	result->sp_min = (long int) atol(PQgetvalue(res, 0, SHADOW_MIN));
	result->sp_max = (long int) atol(PQgetvalue(res, 0, SHADOW_MAX));
	result->sp_warn = (long int) atol(PQgetvalue(res, 0, SHADOW_WARN));
	result->sp_inact = (long int) atol(PQgetvalue(res, 0, SHADOW_INACT));
	result->sp_expire = (long int) atol(PQgetvalue(res, 0, SHADOW_EXPIRE));
	result->sp_flag = (unsigned long int) atol(PQgetvalue(res, 0, SHADOW_FLAG));

	DebugPrint("Converted a res to a pwd:\n");
	DebugPrint("Name: %s\n" _C_ result->sp_namp);
	DebugPrint("Password: %s\n" _C_ result->sp_pwdp);
	DebugPrint("lastchange: %d\n" _C_ result->sp_lstchg);
	DebugPrint("min: %d\n" _C_ result->sp_min);
	DebugPrint("max: %d\n" _C_ result->sp_max);
	DebugPrint("warn: %d\n" _C_ result->sp_warn);
	DebugPrint("inact: %d\n" _C_ result->sp_inact);
	DebugPrint("expire: %d\n" _C_ result->sp_expire);
	DebugPrint("flag: %d\n" _C_ result->sp_flag);

BAIL_OUT:
	return status;
}

/*
 * get a passwd entry from cursor
 */
enum nss_status backend_getspent(struct spwd *result, char *buffer,
                                 size_t buflen, int *errnop)
{
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;

	res = fetch("shadow");
	if(PQresultStatus(res) == PGRES_TUPLES_OK) {
		status = res2shadow(res, result, buffer, buflen, errnop);
	}
	if (status == NSS_STATUS_TRYAGAIN && *errnop == ERANGE) {
		res = putback("shadow");
		PQclear(res);
	}
	PQclear(res);
	return status;
}    

/*
 * backend for getspnam()
 */
enum nss_status backend_getspnam(const char *name, struct spwd *result,
                                 char *buffer, size_t buflen, int *errnop)
{
	const char *params[1];
	PGresult *res;
	enum nss_status status = NSS_STATUS_NOTFOUND;

	params[0] = name;

	res = PQexecParams(_shadowconn, getcfg("shadowbyname"), 1, NULL, params, NULL, NULL, 0);

	if(PQresultStatus(res) == PGRES_TUPLES_OK) {
		status = res2shadow(res, result, buffer, buflen, errnop);
	}
	PQclear(res);
    
	return status;
}

