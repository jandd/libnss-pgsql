lib_LTLIBRARIES = libnss_pgsql.la
libnss_pgsql_la_SOURCES = interface.c config.c backend.c util.c interface-shadow.c config-shadow.c backend-shadow.c
libnss_pgsql_la_LDFLAGS = -module -version-info 2
EXTRA_DIST = nss-pgsql.h

INCLUDES = -DLIBDIR=\"$(libdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -D_GNU_SOURCE
CFLAGS = -O2 -Wall -Wstrict-prototypes
