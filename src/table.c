#include "table.h"

PGTable * getTable(PGconn *c, PGROption *opts)
{
	PGTable	*t;
	PGresult	*res;
	int *n;

	char *query = "SELECT c.oid, n.nspname, c.relname, c.relkind, t.spcname AS tablespacename, 'p' AS relpersistence, array_to_string(c.reloptions, ', ') AS reloptions, obj_description(c.oid, 'pg_class') AS description, pg_get_userbyid(c.relowner) AS relowner, relacl, 'v' AS relreplident, 0 AS reloftype, NULL AS typnspname, NULL AS typname, false AS relispartition, NULL AS partitionkeydef, NULL AS partitionbound, c.relhassubclass FROM pg_class c INNER JOIN pg_namespace n ON (c.relnamespace = n.oid) LEFT JOIN pg_tablespace t ON (c.reltablespace = t.oid) WHERE relkind = 'r' AND n.nspname !~ '^pg_' AND n.nspname <> 'information_schema' AND n.nspname = '%s' AND c.relname = '%s';";
	asprintf(&query, opts->schema, opts->table);
	res = PQexec(c, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		//logError("query failed: %s", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	*n = PQntuples(res);
	if (*n > 0)
		t = (PGTable *) malloc(*n * sizeof(PGTable));
	else
		t = NULL;

	/*data for table*/
	t->oid = strtoul(PQgetvalue(res, 0, PQfnumber(res, "oid")), NULL, 10);
	t->schema = strdup(PQgetvalue(res, 0, PQfnumber(res, "nspname")));
	t->table = strdup(PQgetvalue(res, 0, PQfnumber(res, "relname")));

	PQclear(res);
	return t;
}