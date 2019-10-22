#include "view.h"

void getDependentViews(PGconn *c, PGTable *t)
{
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"select n1.oid, n1.schema, n1.view, n1.relkind, vw.view_definition, relowner, comment, acl from(select distinct v.relname as table, t.oid, nv.nspname as schema, t.relname AS view, t.relkind, pg_get_userbyid(t.relowner) AS relowner, obj_description(t.oid, 'pg_class') AS comment, t.relacl as acl from pg_class v left join  pg_depend dv ON dv.refobjid = v.oid LEFT JOIN pg_depend dt ON dv.classid = dt.classid AND dv.objid = dt.objid AND dv.refobjid <> dt.refobjid AND dv.refclassid = dt.refclassid AND dv.classid = 'pg_catalog.pg_rewrite'::regclass AND dv.refclassid = 'pg_catalog.pg_class'::regclass LEFT JOIN pg_class t ON t.oid = dt.refobjid LEFT JOIN pg_namespace nv ON t.relnamespace = nv.oid where v.oid = %u AND dt.deptype = 'i' AND t.relkind IN ('v', 'm') )n1 inner join information_schema.views vw ON (vw.table_schema = n1.schema AND vw.table_name = n1.view) order by n1.oid", t->oid);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nviews = PQntuples(res);
	if (t->nviews > 0)
		t->views = (PGView *) malloc(t->nviews * sizeof(PGView));
	else
		t->views = NULL;

	for (i = 0; i < t->nviews; i++)
	{
		t->views[i].oid = atoi(PQgetvalue(res, i, PQfnumber(res, "oid")));
		t->views[i].schema = PQgetvalue(res, i, PQfnumber(res, "schema"));
		t->views[i].view = PQgetvalue(res, i, PQfnumber(res, "view"));
		t->views[i].relkind = PQgetvalue(res, i, PQfnumber(res, "relkind"));
		t->views[i].view_definition = PQgetvalue(res, i, PQfnumber(res, "view_definition"));
		t->views[i].relowner = PQgetvalue(res, i, PQfnumber(res, "relowner"));
		t->views[i].comment = PQgetvalue(res, i, PQfnumber(res, "comment"));
		t->views[i].acl = PQgetvalue(res, i, PQfnumber(res, "acl"));
	}
}