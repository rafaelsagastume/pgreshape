/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 *  UNSUPPORTED
 * ~~~~~~~~~~~
 * columns (comment, default, type, security, check, unique, constraint)
 * views
 * functions
 * index
 * constraints (unique, check, not-null, primary, foreign keys, exclusion key)
 * procedure
 * triggers
 * materialized views
 * rules
 * comments
 * acl
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#include "pgreshape.h"
#include "config.h"
#include "common.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void help(void);
PGconn* pg_connect(const char * host, const char *user, const char *pass, const char *dbname, const char *port);
static void pg_close(PGconn *conn);


/*
 * helper to connect to the database
 */
PGconn* pg_connect(const char * host, const char *user, const char *pass, const char *dbname, const char *port) {
	PGconn *conn = NULL;
	ConnStatusType status;
	conn = PQsetdbLogin(host, port, NULL, NULL, dbname, user, pass);
	if((status = PQstatus(conn)) != CONNECTION_OK) {
		printf("%s\n", PQerrorMessage(conn));
		conn = NULL;
	}
	
	return conn;
}


/*
 * helper to close to the database
 */
static void pg_close(PGconn *conn) {
	if(conn) {
		PQfinish(conn);       
	}
}


static void help(void)
{
	printf("%s: Embed the new column according to the desired position in any table.\n\n", PGR_NAME);
}


int main(int argc, char const *argv[])
{
	ConfigFile *config = NULL;
	PGROption *opts = NULL;
	PGconn *conn = NULL;

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0)
		{
			help();
			exit(EXIT_SUCCESS);
		}
		
		if (strcmp(argv[1], "--version") == 0)
		{
			printf(PGR_NAME " " PGR_VERSION "\n");
			exit(EXIT_SUCCESS);
		}

		if (strcmp(argv[1], "-c") == 0)
		{
			config = loadConfig(argv[2]);
		}
	}


	if(config!=NULL) {

		opts = (PGROption*)malloc(1 * sizeof(PGROption));

		while ((argc > 1)) {

			if (!strcmp(argv[1], "-s")){

				if (argv[2])
					opts->schema = strdup(argv[2]);

			} else if (!strcmp(argv[1], "-t")) {

				if (argv[2])
					opts->table = strdup(argv[2]);

			} else if (!strcmp(argv[1], "-offset")) {

				if (argv[2])
					opts->offset = strdup(argv[2]);

			} else if (!strcmp(argv[1], "-column")) {

				if (argv[2])
					opts->column = strdup(argv[2]);

			} else if (!strcmp(argv[1], "-type")) {

				if (argv[2])
					opts->type = strdup(argv[2]);

			}

			++argv;
			--argc;
		}


		/*
		* database connection is prepared
		*/
		conn = pg_connect(config->host, config->user, config->password, config->dbname, config->port);

		

		pg_close(conn);
	} else {
		printf("nothing to do.\n");
	}

	freeConfig(config);
	return 0;
}