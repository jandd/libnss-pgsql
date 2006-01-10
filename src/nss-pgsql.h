#ifndef __NSS_PGSQL_H_INCLUDED__
#  define __NSS_PGSQL_H_INCLUDED__

#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  endif

#  ifdef HAVE_UNISTD_H
#    include <unistd.h>
#  endif

#  ifdef HAVE_NSS_H
#    include <nss.h>
#  endif

#  include <pwd.h>
#  include <grp.h>
#  include <shadow.h>
#  include <sys/types.h>

#define CFGFILE SYSCONFDIR"/nss-pgsql.conf"
#define CFGROOTFILE SYSCONFDIR"/nss-pgsql-root.conf"

/* define backend connection types */
#define CONNECTION_SHADOW    's'
#define CONNECTION_USERGROUP 'n'

int readconfig(char type, char* configfile);
void cleanup(void);
char *getcfg(const char *key);

int backend_isopen(char type);
int backend_open(char type);
void backend_close(char type);
enum nss_status getent_prepare(const char *what);
void getent_close(char type);

enum nss_status backend_getpwent(struct passwd *result,
											char *buffer,
											size_t buflen,
											int *errnop);
enum nss_status backend_getgrent(struct group *result,
											char *buffer,
											size_t buflen,
											int *errnop);
enum nss_status backend_getpwuid(uid_t uid,
											struct passwd *result,
											char *buffer,
											size_t buflen,
											int *errnop);
enum nss_status backend_getgrgid(gid_t gid,
											struct group *result,
											char *buffer,
											size_t buflen,
											int *errnop);
enum nss_status backend_getgrnam(const char *name,
											struct group *result,
											char *buffer,
											size_t buflen,
											int *errnop);
enum nss_status backend_getpwnam(const char *name,
											struct passwd *result,
											char *buffer,
											size_t buflen,
											int *errnop);
size_t backend_initgroups_dyn(const char *user,
										gid_t group,
										long int *start,
										long int *size,
										gid_t **groupsp,
										long int limit,
										int *errnop);

enum nss_status backend_getspent(struct spwd *result,
											char *buffer,
											size_t buflen,
											int *errnop);

enum nss_status backend_getspnam(const char *name,
											struct spwd *result,
											char *buffer,
											size_t buflen,
											int *errnop);

void print_err(const char *msg, ...);
void print_msg(const char *msg, ...);

#define _C_  ,    /// Debug , to simulate vararg macros
#  ifdef DEBUG
#    define DebugPrint(x) print_msg(x)
#  else
#    define DebugPrint(x)
#  endif

#endif
