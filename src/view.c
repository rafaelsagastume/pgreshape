#include "view.h"
#include <stdio.h>

void getDependentViews(PGconn *c, PGTable *t)
{
	PGresult	*res;
	char *query;
	int i;

	asprintf(&query, 
		"SELECT DISTINCT n1.oid, n1.schema, n1.view, n1.relkind, vw.view_definition, relowner, comment, acl, reloptions, checkoption from(select distinct CASE WHEN 'check_option=local' = ANY(t.reloptions) THEN 'LOCAL'::text WHEN 'check_option=cascaded' = ANY(t.reloptions) THEN 'CASCADED'::text ELSE NULL END AS checkoption, array_to_string(t.reloptions, ', ') AS reloptions, v.relname as table, t.oid, nv.nspname as schema, t.relname AS view, t.relkind, pg_get_userbyid(t.relowner) AS relowner, obj_description(t.oid, 'pg_class') AS comment, t.relacl as acl from pg_class v left join  pg_depend dv ON dv.refobjid = v.oid LEFT JOIN pg_depend dt ON dv.classid = dt.classid AND dv.objid = dt.objid AND dv.refobjid <> dt.refobjid AND dv.refclassid = dt.refclassid AND dv.classid = 'pg_catalog.pg_rewrite'::regclass AND dv.refclassid = 'pg_catalog.pg_class'::regclass LEFT JOIN pg_class t ON t.oid = dt.refobjid LEFT JOIN pg_namespace nv ON t.relnamespace = nv.oid where t.oid IN(select unnest(ids::oid[]) from(select (all_objects_id || all_objects_ramas) as ids from(WITH RECURSIVE all_objects_ramas AS (WITH all_objects AS (select n1.oid as id, (n1.schema||'.'||n1.view) as view, oid_root from(select distinct t.oid, nv.nspname as schema, t.relname AS view, v.oid as oid_root from pg_class v left join pg_depend dv ON dv.refobjid = v.oid LEFT JOIN pg_depend dt ON dv.classid = dt.classid AND dv.objid = dt.objid AND dv.refobjid <> dt.refobjid AND dv.refclassid = dt.refclassid AND dv.classid = 'pg_catalog.pg_rewrite'::regclass AND dv.refclassid = 'pg_catalog.pg_class'::regclass  LEFT JOIN pg_class t ON t.oid = dt.refobjid LEFT JOIN pg_namespace nv ON t.relnamespace = nv.oid where dt.deptype = 'i' AND t.relkind IN ('v', 'm') AND nv.nspname !~ '^pg_' AND nv.nspname <> 'information_schema' )n1 inner join information_schema.views vw ON (vw.table_schema = n1.schema AND vw.table_name = n1.view)) SELECT /*solo padres*/ id, oid_root, view, ARRAY[view] AS chain_of_command_names, ARRAY[id] AS chain_of_command_ids, 1 AS level FROM all_objects UNION SELECT /*recursive*/e.id, e.oid_root, e.view, array_append(s.chain_of_command_names, e.view) AS chain_of_command_names, array_append(s.chain_of_command_ids, e.id) AS chain_of_command_ids, s.level + 1 AS level FROM all_objects AS e JOIN all_objects_ramas AS s ON s.id = e.oid_root) SELECT array_remove(array_agg(a.id), NULL) AS all_objects_id, array_remove(array_agg(b.id), NULL) AS all_objects_ramas FROM all_objects_ramas AS a LEFT JOIN all_objects_ramas AS b ON a.id = ANY (b.chain_of_command_ids) AND a.level < b.level WHERE a.oid_root = %u ORDER BY 1 )n2 ) as n3 ) AND dt.deptype = 'i' AND t.relkind IN ('v', 'm') )n1 inner join information_schema.views vw ON (vw.table_schema = n1.schema AND vw.table_name = n1.view) order by n1.oid ASC", t->oid);

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



void getViewSecurityLabels(PGconn *c, PGTable *t) {
	PGresult	*res;
	char *query = NULL;
	int i;

	if (PQserverVersion(c) < 90100)
	{
		printf("ignoring security labels because server does not support it");
		return;
	}

	for (i = 0; i < t->nviews; i++)
	{

		int j; 

		asprintf(&query, 
			"SELECT provider, label FROM pg_seclabel s INNER JOIN pg_class c ON (s.classoid = c.oid) WHERE c.relname = 'pg_class' AND s.objoid = %u ORDER BY provider", t->views[i].oid);

		res = PQexec(c, query);
		
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			printf("query failed: %s\n", PQresultErrorMessage(res));
			PQclear(res);
			PQfinish(c);
			exit(EXIT_FAILURE);
		}

		t->views[i].nseclabels = PQntuples(res);
		if (t->views[i].nseclabels > 0)
			t->views[i].seclabels = (PGSecLabel *) malloc(t->views[i].nseclabels * sizeof(PGSecLabel));
		else
			t->views[i].seclabels = NULL;

		for (j = 0; j < t->views[i].nseclabels; j++)
		{
			char *withoutescape;
			t->views[i].seclabels[j].provider = strdup(PQgetvalue(res, j, PQfnumber(res, "provider")));
			withoutescape = PQgetvalue(res, j, PQfnumber(res, "label"));
			t->views[i].seclabels[j].label = PQescapeLiteral(c, withoutescape, strlen(withoutescape));
			if (t->views[i].seclabels[j].label == NULL)
			{
				printf("[view] escaping label failed: %s", PQerrorMessage(c));
				PQclear(res);
				PQfinish(c);
				exit(EXIT_FAILURE);
			}
		}

		PQclear(res);
	}
}



void dumpDropDependentView(FILE *fout, PGTable *t) {
	int i;
	for (i = t->nviews-1; i > -1; i--)
	{
		fprintf(fout, "DROP VIEW IF EXISTS %s.%s;\n", t->views[i].schema, t->views[i].view);
	}
}


void dumpCreateCreateView(FILE *fout, PGTable *t) {
	char *owner;
	int i;
	for (i = 0; i < t->nviews; i++)
	{
		/*generate view*/
		fprintf(fout, "\n\n");
		fprintf(fout, "CREATE ");

		if (t->views[i].relkind == 'v')
		{
			fprintf(fout, "VIEW ");
		} else if (t->views[i].relkind == 'm') {
			fprintf(fout, "MATERIALIZED VIEW ");
		}
 
		fprintf(fout, "%s.%s", t->views[i].schema, t->views[i].view);

		/* reloptions */
		if (t->views[i].reloptions != NULL)
			fprintf(fout, " WITH (%s)", t->views[i].reloptions);


		fprintf(fout, " AS \n%s\n", t->views[i].view_definition);


		if (t->views[i].checkoption != NULL)
			fprintf(fout, "\n WITH %s CHECK OPTION", t->views[i].checkoption);

		/*generate owner*/
		owner = formatObjectIdentifier(t->views[i].relowner);
		fprintf(fout, "ALTER VIEW %s.%s OWNER TO %s;\n", t->views[i].schema, t->views[i].view, t->views[i].relowner);
		free(owner);


		/*generate acl on view*/
		dumpGrant(fout, OBTable, t->views[i].schema, t->views[i].view, t->views[i].acl, NULL);
		fprintf(fout, "\n");

		/*generate comment to views*/
		fprintf(fout, "COMMENT ON ");
		if (t->views[i].relkind == 'v')
		{
			fprintf(fout, "VIEW ");
		} else if (t->views[i].relkind == 'm') {
			fprintf(fout, "MATERIALIZED VIEW ");
		}

		fprintf(fout, "%s.%s IS '%s';\n", t->views[i].schema, t->views[i].view, t->views[i].comment);


		if (t->views[i].nseclabels > 0)
		{

			int j;
			fprintf(fout, "\n\n");
			fprintf(fout, "-- \n");
			fprintf(fout, "-- the security labels on view [%s.%s]\n", t->views[i].schema, t->views[i].view);
			fprintf(fout, "-- \n");
			for (j = 0; j < t->views[i].nseclabels; ++j)
			{
				fprintf(fout, "SECURITY LABEL FOR %s ON VIEW %s.%s IS %s;\n",
						t->views[i].seclabels[j].provider,
						t->views[i].schema,
						t->views[i].view,
						t->views[i].seclabels[j].label);
			}
			
		}

	}

}
