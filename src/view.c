#include "view.h"

void getDependentViews(PGconn *c, PGTable *t)
{
	PGresult	*res;
	char *query = NULL;
	int i;

	asprintf(&query, 
		"select n1.oid, n1.schema, n1.view, n1.relkind, vw.view_definition, relowner, comment, acl, reloptions, checkoption from(select distinct CASE WHEN 'check_option=local' = ANY(t.reloptions) THEN 'LOCAL'::text WHEN 'check_option=cascaded' = ANY(t.reloptions) THEN 'CASCADED'::text ELSE NULL END AS checkoption, array_to_string(t.reloptions, ', ') AS reloptions, v.relname as table, t.oid, nv.nspname as schema, t.relname AS view, t.relkind, pg_get_userbyid(t.relowner) AS relowner, obj_description(t.oid, 'pg_class') AS comment, t.relacl as acl from pg_class v left join  pg_depend dv ON dv.refobjid = v.oid LEFT JOIN pg_depend dt ON dv.classid = dt.classid AND dv.objid = dt.objid AND dv.refobjid <> dt.refobjid AND dv.refclassid = dt.refclassid AND dv.classid = 'pg_catalog.pg_rewrite'::regclass AND dv.refclassid = 'pg_catalog.pg_class'::regclass LEFT JOIN pg_class t ON t.oid = dt.refobjid LEFT JOIN pg_namespace nv ON t.relnamespace = nv.oid where v.oid = %u AND dt.deptype = 'i' AND t.relkind IN ('v', 'm') )n1 inner join information_schema.views vw ON (vw.table_schema = n1.schema AND vw.table_name = n1.view) order by n1.oid", t->oid);

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
		t->views[i].schema = strdup(PQgetvalue(res, i, PQfnumber(res, "schema")));
		t->views[i].view = strdup(PQgetvalue(res, i, PQfnumber(res, "view")));
		t->views[i].relkind = PQgetvalue(res, i, PQfnumber(res, "relkind"))[0];
		t->views[i].view_definition = strdup(PQgetvalue(res, i, PQfnumber(res, "view_definition")));
		t->views[i].relowner = strdup(PQgetvalue(res, i, PQfnumber(res, "relowner")));
		t->views[i].comment = strdup(PQgetvalue(res, i, PQfnumber(res, "comment")));
		t->views[i].acl = strdup(PQgetvalue(res, i, PQfnumber(res, "acl")));

		if (PQgetisnull(res, i, PQfnumber(res, "reloptions")))
			t->views[i].reloptions = NULL;
		else
			t->views[i].reloptions = strdup(PQgetvalue(res, i, PQfnumber(res, "reloptions")));

		if (PQgetisnull(res, i, PQfnumber(res, "checkoption")))
			t->views[i].checkoption = NULL;
		else
			t->views[i].checkoption = strdup(PQgetvalue(res, i, PQfnumber(res, "checkoption")));
	}
}


void dumpDropDependentView(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nviews; i++)
	{
		fprintf(fout, "DROP VIEW IF EXISTS %s.%s;\n", t->views[i].schema, t->views[i].view);
	}
}


void dumpCreateCreateView(FILE *fout, PGTable *t) {

	int i;
	for (i = 0; i < t->nviews; i++)
	{
		/*generate view*/
		fprintf(fout, "\n");
		fprintf(fout, "CREATE ");

		if (t->views[i].relkind == 'v')
		{
			fprintf(fout, "VIEW ");
		} else if (t->views[i].relkind == 'm') {
			fprintf(fout, "MATERIALIZED VIEW ");
		}
 
		fprintf(fout, "IF NOT EXISTS ");
		fprintf(fout, "%s.%s", t->views[i].schema, t->views[i].view);

		/* reloptions */
		if (t->views[i].reloptions != NULL)
			fprintf(fout, " WITH (%s)", t->views[i].reloptions);


		fprintf(fout, " AS \n%s\n", t->views[i].view_definition);


		if (t->views[i].checkoption != NULL)
			fprintf(fout, "\n WITH %s CHECK OPTION", t->views[i].checkoption);

		/*generate owner*/
		char	*owner = formatObjectIdentifier(t->views[i].relowner);
		fprintf(fout, "ALTER VIEW %s.%s OWNER TO %s;\n", t->views[i].schema, t->views[i].view, t->views[i].relowner);
		free(owner);


		/*generate comment to views*/
		fprintf(fout, "COMMENT ON ");
		if (t->views[i].relkind == 'v')
		{
			fprintf(fout, "VIEW ");
		} else if (t->views[i].relkind == 'm') {
			fprintf(fout, "MATERIALIZED VIEW ");
		}

		fprintf(fout, "%s.%s IS '%s';\n", t->views[i].schema, t->views[i].view, t->views[i].comment);

	}

}
