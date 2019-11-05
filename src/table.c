#include "table.h"

PGTable * getTable(PGconn *c, PGROption *opts)
{
	PGTable	*t;
	PGresult	*res;
	int n;
	char *query = NULL;

	asprintf(&query,
		"SELECT c.oid, n.nspname, c.relname, c.relkind, t.spcname AS tablespacename, 'p' AS relpersistence, array_to_string(c.reloptions, ', ') AS reloptions, obj_description(c.oid, 'pg_class') AS description, pg_get_userbyid(c.relowner) AS relowner, relacl, 'v' AS relreplident, 0 AS reloftype, NULL AS typnspname, NULL AS typname, false AS relispartition, NULL AS partitionkeydef, NULL AS partitionbound, c.relhassubclass FROM pg_class c INNER JOIN pg_namespace n ON (c.relnamespace = n.oid) LEFT JOIN pg_tablespace t ON (c.reltablespace = t.oid) WHERE relkind = 'r' AND n.nspname !~ '^pg_' AND n.nspname <> 'information_schema' AND n.nspname = '%s' AND c.relname = '%s';", opts->schema, opts->table);
	
	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	n = PQntuples(res);
	if (n > 0) {
		t = (PGTable *) malloc(n * sizeof(PGTable));

		/*data for table*/
		t->oid = strtoul(PQgetvalue(res, 0, PQfnumber(res, "oid")), NULL, 10);
		t->schema = strdup(PQgetvalue(res, 0, PQfnumber(res, "nspname")));
		t->table = strdup(PQgetvalue(res, 0, PQfnumber(res, "relname")));
	} else
		t = NULL;

	getPrimaryKeys(c, t);

	PQclear(res);
	return t;
}


void getPrimaryKeys(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int n;

	asprintf(&query, 
		"SELECT array_to_string(array_agg(kcu.column_name::text), ', ')primarys, array_to_string(array_agg('a.'||kcu.column_name::text), ', ')primarys_aa, array_to_string(array_agg('n.'||kcu.column_name::text), ', ')primarys_nn FROM information_schema.table_constraints tc LEFT JOIN information_schema.key_column_usage kcu ON tc.constraint_catalog = kcu.constraint_catalog AND tc.constraint_schema = kcu.constraint_schema AND tc.constraint_name = kcu.constraint_name WHERE lower(tc.constraint_type) = ('primary key') AND tc.table_schema = '%s' AND tc.table_name = '%s' GROUP BY kcu.column_name", t->schema, t->table);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	n = PQntuples(res);
	if (n > 0) {
		t->primary_keys = strdup(PQgetvalue(res, 0, PQfnumber(res, "primarys")));
		t->primary_keys_nn = strdup(PQgetvalue(res, 0, PQfnumber(res, "primarys_nn")));
		t->primary_keys_aa = strdup(PQgetvalue(res, 0, PQfnumber(res, "primarys_aa")));
	} else {
		t->primary_keys = NULL;
		t->primary_keys_nn = NULL;
		t->primary_keys_aa = NULL;
	}
}



void getTableAttributes(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT a.attnum, a.attname, a.attnotnull, pg_catalog.format_type(t.oid, a.atttypmod) as atttypname, pg_get_expr(d.adbin, a.attrelid) as attdefexpr, CASE WHEN a.attcollation <> t.typcollation THEN c.collname ELSE NULL END AS attcollation, col_description(a.attrelid, a.attnum) AS description, a.attstattarget, a.attstorage, CASE WHEN t.typstorage <> a.attstorage THEN FALSE ELSE TRUE END AS defstorage, array_to_string(attoptions, ', ') AS attoptions, attacl FROM pg_attribute a LEFT JOIN pg_type t ON (a.atttypid = t.oid) LEFT JOIN pg_attrdef d ON (a.attrelid = d.adrelid AND a.attnum = d.adnum) LEFT JOIN pg_collation c ON (a.attcollation = c.oid) WHERE a.attrelid = %u AND a.attnum > 0 AND attisdropped IS FALSE ORDER BY a.attnum", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nattributes = PQntuples(res);
	if (t->nattributes > 0)
		t->attributes = (PGAttribute *) malloc(t->nattributes * sizeof(PGAttribute));
	else
		t->attributes = NULL;

	for (i = 0; i < t->nattributes; i++)
	{
		char storage;
		char *withoutescape;

		t->attributes[i].attnum = atoi(PQgetvalue(res, i, PQfnumber(res, "attnum")));
		t->attributes[i].attname = strdup(PQgetvalue(res, i, PQfnumber(res, "attname")));
		t->attributes[i].attnotnull = (PQgetvalue(res, i, PQfnumber(res, "attnotnull"))[0] == 't');
		t->attributes[i].atttypname = strdup(PQgetvalue(res, i, PQfnumber(res, "atttypname")));

		if (PQgetisnull(res, i, PQfnumber(res, "attdefexpr"))) {
			t->attributes[i].attdefexpr = NULL;
		} else {
			t->attributes[i].attdefexpr = strdup(PQgetvalue(res, i, PQfnumber(res, "attdefexpr")));
		}

		t->attributes[i].attstattarget = atoi(PQgetvalue(res, i, PQfnumber(res, "attstattarget")));

		storage = PQgetvalue(res, i, PQfnumber(res, "attstorage"))[0];
		switch (storage)
		{
			case 'p':
				t->attributes[i].attstorage = strdup("PLAIN");
				break;
			case 'e':
				t->attributes[i].attstorage = strdup("EXTERNAL");
				break;
			case 'm':
				t->attributes[i].attstorage = strdup("MAIN");
				break;
			case 'x':
				t->attributes[i].attstorage = strdup("EXTENDED");
				break;
			default:
				t->attributes[i].attstorage = NULL;
				break;
		}

		t->attributes[i].defstorage = (PQgetvalue(res, i, PQfnumber(res, "defstorage"))[0] == 't');

		if (PQgetisnull(res, i, PQfnumber(res, "attcollation")))
			t->attributes[i].attcollation = NULL;
		else
			t->attributes[i].attcollation = strdup(PQgetvalue(res, i, PQfnumber(res, "attcollation")));

		if (PQgetisnull(res, i, PQfnumber(res, "attoptions")))
			t->attributes[i].attoptions = NULL;
		else
			t->attributes[i].attoptions = strdup(PQgetvalue(res, i, PQfnumber(res, "attoptions")));

		if (PQgetisnull(res, i, PQfnumber(res, "attacl")))
			t->attributes[i].acl = NULL;
		else
			t->attributes[i].acl = strdup(PQgetvalue(res, i, PQfnumber(res, "attacl")));


		if (PQgetisnull(res, i, PQfnumber(res, "description")))
			t->attributes[i].comment = NULL;
		else
		{
			withoutescape = PQgetvalue(res, i, PQfnumber(res, "description"));
			t->attributes[i].comment = PQescapeLiteral(c, withoutescape, strlen(withoutescape));
			if (t->attributes[i].comment == NULL)
			{
				printf("escaping comment failed: %s", PQerrorMessage(c));
				PQclear(res);
				PQfinish(c);
				exit(EXIT_FAILURE);
			}
		}
	}

	PQclear(res);
}


void getTableIndexes(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT distinct c.oid, relkind as contype, n.nspname, c.relname, pg_get_indexdef(c.oid) AS indexdef, obj_description(c.oid, 'pg_class') AS description, (case when (select tablespace from pg_indexes where indexname = c.relname limit 1) isnull then (select spcname from pg_tablespace where oid = (select dattablespace from pg_database  where datname = (select catalog_name from information_schema.schemata where schema_name = n.nspname))) else (select tablespace from pg_indexes where indexname = c.relname limit 1) end) as tablespace FROM pg_class c INNER JOIN pg_namespace n ON (c.relnamespace = n.oid) INNER JOIN pg_index i ON (i.indexrelid = c.oid) inner join pg_depend d ON (d.objid = c.oid) LEFT JOIN pg_tablespace t ON (c.reltablespace = t.oid) WHERE d.refobjid = %u and relkind = 'i' AND nspname !~ '^pg_' AND nspname <> 'information_schema' AND NOT indisprimary ORDER BY nspname, relname;", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nindexes = PQntuples(res);
	if (t->nindexes > 0)
		t->indexes = (PGIndex *) malloc(t->nindexes * sizeof(PGIndex));
	else
		t->indexes = NULL;

	for (i = 0; i < t->nindexes; i++)
	{
		char *withoutescape;

		t->indexes[i].contype = PQgetvalue(res, i, PQfnumber(res, "contype"))[0];

		if (PQgetisnull(res, i, PQfnumber(res, "tablespace")))
			t->indexes[i].tablespace = NULL;
		else
		{
			t->indexes[i].tablespace = strdup(PQgetvalue(res, i, PQfnumber(res, "tablespace")));
		}

		t->indexes[i].schema = strdup(PQgetvalue(res, i, PQfnumber(res, "nspname")));
		t->indexes[i].relname = strdup(PQgetvalue(res, i, PQfnumber(res, "relname")));
		t->indexes[i].indexdef = strdup(PQgetvalue(res, i, PQfnumber(res, "indexdef")));

		if (PQgetisnull(res, i, PQfnumber(res, "description")))
			t->indexes[i].comment = NULL;
		else
		{
			withoutescape = PQgetvalue(res, i, PQfnumber(res, "description"));
			t->indexes[i].comment = PQescapeLiteral(c, withoutescape, strlen(withoutescape));
			if (t->indexes[i].comment == NULL)
			{
				printf("escaping comment failed: %s", PQerrorMessage(c));
				PQclear(res);
				PQfinish(c);
				exit(EXIT_FAILURE);
			}
		}
	}

	PQclear(res);
}


void getTableUnique(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT contype, sp.nspname as schema, c.relname as table, c2.relname as unique_name, (SELECT array_to_string(array_agg(a.attname), ', ') FROM pg_attribute a WHERE a.attrelid = c.oid AND a.attnum in (select unnest(con.conkey))) as columns, /*pg_catalog.pg_get_indexdef(i.indexrelid, 0, true) as indexdef,*/ obj_description(c2.oid) AS description, (case when (select tablespace from pg_indexes where indexname = c2.relname limit 1) isnull then  (select spcname from pg_tablespace where oid = (select dattablespace from pg_database  where datname = (select catalog_name from information_schema.schemata where schema_name = sp.nspname))) else (select tablespace from pg_indexes where indexname = c2.relname limit 1) end) as tablespace FROM pg_catalog.pg_class c inner join pg_catalog.pg_index i ON (c.oid = i.indrelid) inner join pg_catalog.pg_class c2 ON (i.indexrelid = c2.oid AND i.indisprimary <> true) inner join pg_catalog.pg_namespace sp ON (sp.oid = c2.relnamespace) LEFT JOIN pg_catalog.pg_constraint con ON (conrelid = i.indrelid AND conindid = i.indexrelid AND contype IN ('u')) WHERE contype NOTNULL AND c.oid = %u ORDER BY i.indisprimary DESC, i.indisunique DESC, c2.relname;", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nunique = PQntuples(res);
	if (t->nunique > 0)
		t->unique = (PGUnique *) malloc(t->nunique * sizeof(PGUnique));
	else
		t->unique = NULL;

	for (i = 0; i < t->nunique; i++)
	{
		char *withoutescape;

		t->unique[i].contype = PQgetvalue(res, i, PQfnumber(res, "contype"))[0];
		
		if (PQgetisnull(res, i, PQfnumber(res, "tablespace")))
			t->unique[i].tablespace = NULL;
		else
		{
			t->unique[i].tablespace = strdup(PQgetvalue(res, i, PQfnumber(res, "tablespace")));
		}

		t->unique[i].schema = strdup(PQgetvalue(res, i, PQfnumber(res, "schema")));
		t->unique[i].table = strdup(PQgetvalue(res, i, PQfnumber(res, "table")));
		t->unique[i].unique_name = strdup(PQgetvalue(res, i, PQfnumber(res, "unique_name")));
		t->unique[i].columns = strdup(PQgetvalue(res, i, PQfnumber(res, "columns")));
		t->unique[i].comment = strdup(PQgetvalue(res, i, PQfnumber(res, "description")));

		if (PQgetisnull(res, i, PQfnumber(res, "description")))
			t->unique[i].comment = NULL;
		else
		{
			withoutescape = PQgetvalue(res, i, PQfnumber(res, "description"));
			t->unique[i].comment = PQescapeLiteral(c, withoutescape, strlen(withoutescape));
			if (t->unique[i].comment == NULL)
			{
				printf("escaping comment failed: %s", PQerrorMessage(c));
				PQclear(res);
				PQfinish(c);
				exit(EXIT_FAILURE);
			}
		}
	}

	PQclear(res);
}



void getTableForeignKey(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT conname, pg_catalog.pg_get_constraintdef(r.oid, true) as condef FROM pg_catalog.pg_constraint r INNER JOIN pg_catalog.pg_class c ON (c.oid = r.conrelid) INNER JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace WHERE r.conrelid = %u AND r.contype = 'f' ORDER BY 1", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nforeignkey = PQntuples(res);
	if (t->nforeignkey > 0)
		t->foreignkeys = (PGForeignKey *) malloc(t->nforeignkey * sizeof(PGForeignKey));
	else
		t->foreignkeys = NULL;

	for (i = 0; i < t->nforeignkey; i++)
	{
		t->foreignkeys[i].conname = strdup(PQgetvalue(res, i, PQfnumber(res, "conname")));
		t->foreignkeys[i].condef = strdup(PQgetvalue(res, i, PQfnumber(res, "condef")));
	}

	PQclear(res);
}


void dumpDropIndex(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nindexes; i++)
	{
		fprintf(fout, "DROP INDEX %s.%s;\n", t->indexes[i].schema, t->indexes[i].relname);
	}
}


void dumpDropUnique(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nunique; i++)
	{
		fprintf(fout, "ALTER TABLE %s.%s DROP CONSTRAINT %s;\n", t->unique[i].schema, t->unique[i].table, t->unique[i].unique_name);
	}
}


void dumpDropForeignKey(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nforeignkey; i++)
	{
		fprintf(fout, "ALTER TABLE %s.%s DROP CONSTRAINT %s;\n", t->schema, t->table, t->foreignkeys[i].conname);
	}
}


void dumpCreateTempTableBackup(FILE *fout, PGTable *t) {
	fprintf(fout, "CREATE TABLE %s.%s_reshape_bk AS SELECT * FROM %s.%s ORDER BY 1 DESC;\n", t->schema, t->table, t->schema, t->table);
}


void dumpDropTableColumn(FILE *fout, PGTable *t, PGROption *opts) {
	/*search offset column*/
	int offset_column = getAttnumOffset(t, opts);

	int i;
	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			fprintf(fout, "ALTER TABLE %s.%s DROP COLUMN %s;\n", t->schema, t->table, t->attributes[i].attname);
		}
	}
}


int getAttnumOffset(PGTable *t, PGROption *opts) {
	int index;
	int attnum = 0;

	if (t->nattributes)
	{
		for (index = 0; index < t->nattributes; ++index)
		{
			if (strcmp(t->attributes[index].attname, opts->offset) == 0)
			{
				attnum = t->attributes[index].attnum;
				break;
			}
		}
	}

	return attnum;
}


int existsColumn(PGTable *t, char *column) {
	int index;
	int attnum = 0;

	if (t->nattributes)
	{
		for (index = 0; index < t->nattributes; ++index)
		{
			if (strcmp(t->attributes[index].attname, column) == 0)
			{
				attnum = t->attributes[index].attnum;
				break;
			}
		}
	}

	return attnum;
}


void dumpNewColumn(FILE *fout, PGTable *t, PGROption *opts) {

	fprintf(fout, "ALTER TABLE %s.%s ADD COLUMN %s %s;\n", t->schema, t->table, opts->column, opts->type);

}


void dumpColumnTable(FILE *fout, PGTable *t, PGROption *opts) {
	/*search offset column*/
	int offset_column = getAttnumOffset(t, opts);

	int i;
	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			fprintf(fout, "ALTER TABLE %s.%s ADD COLUMN %s %s", t->schema, t->table, t->attributes[i].attname, t->attributes[i].atttypname);
			
			if (t->attributes[i].attcollation != NULL)
			fprintf(fout, " COLLATE \"%s\"", t->attributes[i].attcollation);

			/* default */
			if (t->attributes[i].attdefexpr != NULL)
				fprintf(fout, " DEFAULT %s", t->attributes[i].attdefexpr);

			/* not null, por motivos del backup, se crean en el ultimo proceso*/

			fprintf(fout, ";\n");
		}
	}
}


void dumpSetNotNullColumnTable(FILE *fout, PGTable *t, PGROption *opts) {
	/*search offset column*/
	int offset_column = getAttnumOffset(t, opts);
	int i;

	fprintf(fout, "\n");
	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			/* not null*/
			if (t->attributes[i].attnotnull) {
				fprintf(fout, "ALTER TABLE %s.%s ALTER COLUMN %s SET NOT NULL;\n", t->schema, t->table, t->attributes[i].attname);
			}
		}
	}
}


void dumpAclColumnTable (FILE *fout, PGTable *t, PGROption *opts) {
	int offset_column = getAttnumOffset(t, opts);
	int i;

	fprintf(fout, "\n");
	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			/* acl */
			if (t->attributes[i].acl) {
				char *attname = formatObjectIdentifier(t->attributes[i].attname);
				dumpGrant(fout, OBTable, t->schema, t->table, t->attributes[i].acl, attname);
				fprintf(fout, "\n");
				free(attname);
			}
		}
	}
}


void dumpOptionsColumnTable (FILE *fout, PGTable *t, PGROption *opts) {
	int offset_column = getAttnumOffset(t, opts);
	int i;

	fprintf(fout, "\n");
	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			/* attoptions */
			if (t->attributes[i].attoptions) {
				char *attname = formatObjectIdentifier(t->attributes[i].attname);

				fprintf(fout, "\n\n");
				fprintf(fout, "ALTER TABLE ONLY %s.%s ALTER COLUMN %s SET (%s);\n", t->schema, t->table, attname, t->attributes[i].attoptions);

				free(attname);
			}
		}
	}
}


void dumpSetCommentColumnTable(FILE *fout, PGTable *t, PGROption *opts) {
	/*search offset column*/
	int offset_column = getAttnumOffset(t, opts);
	int i;

	fprintf(fout, "\n");
	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			/* not null*/
			if (t->attributes[i].comment) {
				fprintf(fout, "COMMENT ON COLUMN %s.%s.%s IS %s;\n", t->schema, t->table, t->attributes[i].attname, t->attributes[i].comment);
			}
		}
	}
}


void dumpCreateForeignKey(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nforeignkey; i++)
	{
		fprintf(fout, "ALTER TABLE ONLY %s.%s\n", t->schema, t->table);
		fprintf(fout, "\tADD CONSTRAINT %s %s", t->foreignkeys[i].conname, t->foreignkeys[i].condef);
		fprintf(fout, ";\n");
	}
}

void dumpCreateIndex(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nindexes; i++)
	{
		fprintf(fout, "\n%s\n", t->indexes[i].indexdef);

		if (t->indexes[i].tablespace) {
			fprintf(fout, "\tTABLESPACE %s;\n", t->indexes[i].tablespace);
		} else
			fprintf(fout, ";\n");

		if (t->indexes[i].comment) {
			fprintf(fout, "\n\n");
			fprintf(fout, "COMMENT ON INDEX %s.%s IS %s;\n",
					t->indexes[i].schema, t->indexes[i].relname, t->indexes[i].comment);
		}
	}
}


void dumpCreateUnique(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nunique; i++)
	{
		fprintf(fout, "\nALTER TABLE %s.%s\n", t->unique[i].schema, t->unique[i].table);
		fprintf(fout, "\tADD CONSTRAINT %s UNIQUE (%s)\n", t->unique[i].unique_name, t->unique[i].columns);

		if (t->unique[i].tablespace) {
			fprintf(fout, "\tUSING INDEX TABLESPACE %s;\n", t->unique[i].tablespace);
		} else
			fprintf(fout, ";\n");

		if (t->unique[i].comment) {
			fprintf(fout, "\n");
			fprintf(fout, "COMMENT ON INDEX %s.%s IS %s;\n",
					t->unique[i].schema, t->unique[i].unique_name, t->unique[i].comment);
		}
	}
}

void getTableCheckConstraint(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT conname, pg_catalog.pg_get_constraintdef(r.oid, true) as condef FROM pg_catalog.pg_constraint r INNER JOIN pg_catalog.pg_class c ON (c.oid = r.conrelid) INNER JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace WHERE r.conrelid = %u AND r.contype = 'c' ORDER BY 1", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->ncheck = PQntuples(res);
	if (t->ncheck > 0)
		t->checks = (PGCheckConstraint *) malloc(t->ncheck * sizeof(PGCheckConstraint));
	else
		t->checks = NULL;

	for (i = 0; i < t->ncheck; i++)
	{
		t->checks[i].conname = strdup(PQgetvalue(res, i, PQfnumber(res, "conname")));
		t->checks[i].condef = strdup(PQgetvalue(res, i, PQfnumber(res, "condef")));
	}

	PQclear(res);
}



void dumpDropCheckConstraint(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->ncheck; i++)
	{
		fprintf(fout, "ALTER TABLE %s.%s DROP CONSTRAINT %s;\n", t->schema, t->table, t->checks[i].conname);
	}
}


void dumpCreateCheckConstraint(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->ncheck; i++)
	{
		fprintf(fout, "ALTER TABLE ONLY %s.%s\n", t->schema, t->table);
		fprintf(fout, "\tADD CONSTRAINT %s %s", t->checks[i].conname, t->checks[i].condef);
		fprintf(fout, ";\n");
	}
}


int existsTable(PGconn *c, char *schema, char *table)
{
	PGresult *res;
	int n;
	char *query = NULL;

	asprintf(&query,
		"select table_name from information_schema.tables where table_schema = '%s' and table_name = '%s';", schema, table);
	
	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}
	
	n = PQntuples(res);
	PQclear(res);
	return n;
}


void getExcludeConstraint(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT conname, pg_catalog.pg_get_constraintdef(r.oid, true) as condef FROM pg_catalog.pg_constraint r INNER JOIN pg_catalog.pg_class c ON (c.oid = r.conrelid) INNER JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace WHERE r.conrelid = %u AND r.contype = 'x' ORDER BY 1", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nexcludec = PQntuples(res);
	if (t->nexcludec > 0)
		t->excludec = (PGExcludeKey *) malloc(t->nexcludec * sizeof(PGExcludeKey));
	else
		t->excludec = NULL;

	for (i = 0; i < t->nexcludec; i++)
	{
		t->excludec[i].conname = strdup(PQgetvalue(res, i, PQfnumber(res, "conname")));
		t->excludec[i].condef = strdup(PQgetvalue(res, i, PQfnumber(res, "condef")));
	}

	PQclear(res);
}

void dumpDropExcludeConstraint(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nexcludec; i++)
	{
		fprintf(fout, "ALTER TABLE %s.%s DROP CONSTRAINT %s;\n", t->schema, t->table, t->excludec[i].conname);
	}
}

void dumpCreateExcludeConstraint(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nexcludec; i++)
	{
		fprintf(fout, "ALTER TABLE ONLY %s.%s\n", t->schema, t->table);
		fprintf(fout, "\tADD CONSTRAINT %s %s", t->excludec[i].conname, t->excludec[i].condef);
		fprintf(fout, ";\n");
	}
}
