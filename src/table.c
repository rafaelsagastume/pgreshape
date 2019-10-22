#include "table.h"

PGTable * getTable(PGconn *c, PGROption *opts)
{
	PGTable	*t;
	PGresult	*res;
	int n;
	char *query = NULL;

	asprintf(&query, "SELECT c.oid, n.nspname, c.relname, c.relkind, t.spcname AS tablespacename, 'p' AS relpersistence, array_to_string(c.reloptions, ', ') AS reloptions, obj_description(c.oid, 'pg_class') AS description, pg_get_userbyid(c.relowner) AS relowner, relacl, 'v' AS relreplident, 0 AS reloftype, NULL AS typnspname, NULL AS typname, false AS relispartition, NULL AS partitionkeydef, NULL AS partitionbound, c.relhassubclass FROM pg_class c INNER JOIN pg_namespace n ON (c.relnamespace = n.oid) LEFT JOIN pg_tablespace t ON (c.reltablespace = t.oid) WHERE relkind = 'r' AND n.nspname !~ '^pg_' AND n.nspname <> 'information_schema' AND n.nspname = '%s' AND c.relname = '%s';", opts->schema, opts->table);
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

	PQclear(res);
	return t;
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
		t->attributes[i].attname = PQgetvalue(res, i, PQfnumber(res, "attname"));
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
		"SELECT contype, c2.relname, pg_catalog.pg_get_indexdef(i.indexrelid, 0, true) FROM pg_catalog.pg_class c inner join pg_catalog.pg_index i ON (c.oid = i.indrelid) inner join pg_catalog.pg_class c2 ON (i.indexrelid = c2.oid AND i.indisprimary <> true) inner join pg_catalog.pg_namespace sp ON (sp.oid = c2.relnamespace) LEFT JOIN pg_catalog.pg_constraint con ON (conrelid = i.indrelid AND conindid = i.indexrelid AND contype IN ('x')) WHERE c.oid = %u ORDER BY i.indisprimary DESC, i.indisunique DESC, c2.relname;", t->oid);

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
		t->indexes[i].contype = PQgetvalue(res, i, PQfnumber(res, "contype"))[0];
		t->indexes[i].relname = PQgetvalue(res, i, PQfnumber(res, "relname"));
		t->indexes[i].pg_get_indexdef = PQgetvalue(res, i, PQfnumber(res, "pg_get_indexdef"));
	}

	PQclear(res);
}


void getTableUnique(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"SELECT contype, c2.relname, pg_catalog.pg_get_indexdef(i.indexrelid, 0, true) FROM pg_catalog.pg_class c inner join pg_catalog.pg_index i ON (c.oid = i.indrelid) inner join pg_catalog.pg_class c2 ON (i.indexrelid = c2.oid AND i.indisprimary <> true) inner join pg_catalog.pg_namespace sp ON (sp.oid = c2.relnamespace) LEFT JOIN pg_catalog.pg_constraint con ON (conrelid = i.indrelid AND conindid = i.indexrelid AND contype IN ('u')) WHERE c.oid = %u ORDER BY i.indisprimary DESC, i.indisunique DESC, c2.relname;", t->oid);

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
		t->unique[i].contype = PQgetvalue(res, i, PQfnumber(res, "contype"))[0];
		t->unique[i].relname = PQgetvalue(res, i, PQfnumber(res, "relname"));
		t->unique[i].pg_get_indexdef = PQgetvalue(res, i, PQfnumber(res, "pg_get_indexdef"));
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
		t->foreignkeys[i].conname = PQgetvalue(res, i, PQfnumber(res, "conname"));
		t->foreignkeys[i].condef = PQgetvalue(res, i, PQfnumber(res, "condef"));
	}

	PQclear(res);
}
