-- Default table setup for nss-pgsql

CREATE SEQUENCE group_id MINVALUE 10000 MAXVALUE 2147483647 NO CYCLE;
CREATE SEQUENCE user_id MINVALUE 10000 MAXVALUE 2147483647 NO CYCLE;

CREATE TABLE "group_table" (
	"gid" int4 NOT NULL DEFAULT nextval('group_id'),
	"groupname" character varying(16) NOT NULL,
	"descr" character varying,
	"passwd" character varying(20),
	PRIMARY KEY ("gid")
);

CREATE TABLE "passwd_table" (
	"username" character varying(64) NOT NULL,
	"passwd" character varying(128) NOT NULL,
	"uid" int4 NOT NULL DEFAULT nextval('user_id'),
	"gid" int4 NOT NULL,
	"gecos" character varying(128),
	"homedir" character varying(256) NOT NULL,
	"shell" character varying DEFAULT '/bin/bash' NOT NULL,
	PRIMARY KEY ("username")
);
CREATE UNIQUE INDEX passwd_table_uid ON passwd_table USING btree (uid);


CREATE TABLE "usergroups" (
	"gid" int4 NOT NULL,
	"uid" int4 NOT NULL,
	PRIMARY KEY ("gid", "uid"),
	CONSTRAINT "ug_gid_fkey" FOREIGN KEY ("gid") REFERENCES "group_table"("gid"),
	CONSTRAINT "ug_uid_fkey" FOREIGN KEY ("uid") REFERENCES "passwd_table"("uid")
);
  
CREATE TABLE "shadow_table" (
 	"username" character varying(64) NOT NULL,
 	"passwd" character varying(128) NOT NULL,
 	"lastchange" int4 NOT NULL,
 	"min" int4 NOT NULL,
 	"max" int4 NOT NULL,
 	"warn" int4 NOT NULL,
 	"inact" int4 NOT NULL,
 	"expire" int4 NOT NULL,
 	"flag" int4 NOT NULL,
 	PRIMARY KEY ("username")
);

CREATE AGGREGATE array_accum (anyelement)
(
	sfunc = array_append,
	stype = anyarray,
	initcond = '{}'
);
