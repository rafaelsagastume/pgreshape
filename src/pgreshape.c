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
#include "table.h"
#include "view.h"
#include "update.h"
#include "trigger.h"
#include <libpq-fe.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

FILE *output;
PGTable *t;

static void help(void);
PGconn* pg_connect(const char * host, const char *user, const char *pass, const char *dbname, const char *port);
static void pg_close(PGconn *conn);
static void getTableObjects(PGconn *c, PGROption *opts);


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


static void pgreshape(FILE *fout, PGROption *opts) {
	fprintf(fout, "--\n-- pgreshape %s\n", PGR_VERSION);
	fprintf(fout, "-- by Rafael Garcia Sagastume Inc.\n");
	fprintf(fout, "-- Copyright %s.\n", PGR_COPY);
	fprintf(fout, "--");

	fprintf(fout, "\n\nBEGIN;\n");
	fprintf(fout, "\nSET session_replication_role = replica;\n\n");
	dumpDisableTriggerAll(fout, t);

	/*dump drop unique*/
	dumpDropUnique(fout, t);

	/*dump drop index*/
	dumpDropIndex(fout, t);

	/*dump drop foreign keys*/
	dumpDropForeignKey(fout, t);

	/*dump drop dependent views*/
	dumpDropDependentView(fout, t);

	/*create table temp for backup data*/
	fprintf(fout, "\n");
	dumpCreateTempTableBackup(fout, t);

	
	if (opts->offset == NULL || strcmp(opts->offset, "") == 0)
	{
		fprintf(fout, "Error -> offset not specified\n");
		exit(EXIT_SUCCESS);
	}

	/*dump drop columns on table*/
	dumpDropTableColumn(fout, t, opts);



	/*
	 *
	 * process for reshape the table with its attributes and dependencies
	 *
	 */
	/*create new column*/
	dumpNewColumn(fout, t, opts);

	/*recreate the processed columns*/
	dumpColumnTable(fout, t, opts);

	/*retrieve temporary backup information*/
	fprintf(fout, "\n");
	dumpUpdateData(fout, t, opts);

	/*dump to generate not null on column*/
	dumpSetNotNullColumnTable(fout, t, opts);

	/*dump to generate the foreign keys again*/
	fprintf(fout, "\n");
	dumpCreateForeignKey(fout, t);

	/*dump to generate index*/
	dumpCreateIndex(fout, t);

	/*dump to generate unique constraint*/
	dumpCreateUnique(fout, t);

	/*dump to generate dependent views*/
	dumpCreateCreateView(fout, t);

	fprintf(fout, "\n");
	dumpEnableTriggerAll(fout, t);
	fprintf(fout, "\n\nSET session_replication_role = DEFAULT;\n");

	fprintf(fout, "\n--COMMIT;\n");
	fprintf(fout, "\n--ROLLBACK;\n");
}


/*
 * process to find the dependencies of the table, views, functions, 
 * foreign keys for recessing the new column
 */
static void getTableObjects(PGconn *c, PGROption *opts) {

	t = getTable(c, opts);

	if (t == NULL)
	{
		/* code */
		printf("without results!\n");
		exit(EXIT_SUCCESS);
	}

	/*bring all the attributes of the table*/
	getTableAttributes(c, t);

	/*Search all indexes referenced to the table*/
	getTableIndexes(c, t);

	/*Search all unique key referenced to the table*/
	getTableUnique(c, t);

	/*Search all foreign keys referenced to the table*/
	getTableForeignKey(c, t);

	/*Search all views referenced to the table*/
	getDependentViews(c, t);

	printf("OID:[%d] Tabla:[%s]\n", t->oid, t->table);
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

		if (config->file != NULL)
		{
			output = fopen(config->file, "w");
			if (!output)
			{
				printf("could not open file \"%s\": %s \n", config->file, strerror(errno));
				exit(EXIT_FAILURE);
			}

			/*extract the necessary objects for the first phase of the procedure*/
			getTableObjects(conn, opts);


			/*offset must be different from primary keys*/
			if (getAttnumOffset(t, opts) <= 0)
			{
				printf("offset not specified : \"%s\"\n", opts->offset);
				exit(EXIT_FAILURE);
			}


			if (existsColumn(t, opts->column) != 0)
			{
				printf("the new column really exists : \"%s\"\n", opts->column);
				exit(EXIT_FAILURE);
			}

			pgreshape(output, opts);

		} else {
			printf("output file not specified.\n");	
		}

		pg_close(conn);
	} else {
		printf("nothing to do.\n");
	}

	freeConfig(config);
	return 0;
}