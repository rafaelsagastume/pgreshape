/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#include "sequence.h"

void getSequences(PGconn *c, PGTable *t, PGROption *opts)
{
	PGresult	*res;
	char *query = NULL;
	int i;
	int offset_column = getAttnumOffset(t, opts);

	asprintf(&query, 
		"select n1.*, sq.start_value, sq.increment, sq.minimum_value as minvalue, sq.maximum_value as maxvalue, sq.cycle_option, sq.data_type from(SELECT c.oid, n.nspname, c.relname, obj_description(c.oid, 'pg_class') AS description, pg_get_userbyid(c.relowner) AS relowner, relacl FROM pg_class c INNER JOIN pg_namespace n ON (c.relnamespace = n.oid) INNER JOIN pg_depend d ON (d.objid = c.oid AND d.refobjid = %u) WHERE relkind = 'S' and refobjsubid NOT IN (SELECT DISTINCT a.attnum FROM pg_attribute a JOIN pg_class pgc ON pgc.oid = a.attrelid JOIN pg_namespace sh ON (sh.oid = pgc.relnamespace) LEFT JOIN pg_index i ON (pgc.oid = i.indrelid AND i.indkey[0] = a.attnum) LEFT JOIN pg_description com on (pgc.oid = com.objoid AND a.attnum = com.objsubid) LEFT JOIN pg_attrdef def ON (a.attrelid = def.adrelid AND a.attnum = def.adnum) WHERE  NOT a.attisdropped AND pgc.oid = d.refobjid and coalesce(i.indisprimary,false) = true ORDER BY a.attnum ) and refobjsubid > %d ORDER BY nspname, relname ) n1 inner join information_schema.sequences sq ON (sq.sequence_schema = nspname and sq.sequence_name = relname)", t->oid, offset_column);

	res = PQexec(c, query);
	
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		printf("query failed: %s\n", PQresultErrorMessage(res));
		PQclear(res);
		PQfinish(c);
		exit(EXIT_FAILURE);
	}

	t->nsequence = PQntuples(res);
	if (t->nsequence > 0)
		t->sequence = (PGSequence *) malloc(t->nsequence * sizeof(PGSequence));
	else
		t->sequence = NULL;

	for (i = 0; i < t->nsequence; i++)
	{
		char *withoutescape;

		t->sequence[i].oid = atoi(PQgetvalue(res, i, PQfnumber(res, "oid")));
		t->sequence[i].nspname = strdup(PQgetvalue(res, i, PQfnumber(res, "nspname")));
		t->sequence[i].relname = strdup(PQgetvalue(res, i, PQfnumber(res, "relname")));
		t->sequence[i].relowner = strdup(PQgetvalue(res, i, PQfnumber(res, "relowner")));
		t->sequence[i].relacl = strdup(PQgetvalue(res, i, PQfnumber(res, "relacl")));

		t->sequence[i].start_value = strdup(PQgetvalue(res, i, PQfnumber(res, "start_value")));
		t->sequence[i].increment = strdup(PQgetvalue(res, i, PQfnumber(res, "increment")));
		t->sequence[i].minvalue = strdup(PQgetvalue(res, i, PQfnumber(res, "minvalue")));
		t->sequence[i].maxvalue = strdup(PQgetvalue(res, i, PQfnumber(res, "maxvalue")));
		t->sequence[i].cycle_option = strdup(PQgetvalue(res, i, PQfnumber(res, "cycle_option")));
		t->sequence[i].typname = strdup(PQgetvalue(res, i, PQfnumber(res, "data_type")));
		
		t->sequence[i].comment = strdup(PQgetvalue(res, i, PQfnumber(res, "description")));

		if (PQgetisnull(res, i, PQfnumber(res, "description")))
			t->sequence[i].comment = NULL;
		else
		{
			withoutescape = PQgetvalue(res, i, PQfnumber(res, "description"));
			t->sequence[i].comment = PQescapeLiteral(c, withoutescape, strlen(withoutescape));
			if (t->sequence[i].comment == NULL)
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


void dumpCreateSequences(FILE *fout, PGTable *t) {
	int i;
	for (i = 0; i < t->nsequence; i++)
	{
		char *owner;

		fprintf(fout, "\nCREATE SEQUENCE %s.%s", t->sequence[i].nspname, t->sequence[i].relname);
		fprintf(fout, "\n\t\tINCREMENT %s", t->sequence[i].increment);
		
		fprintf(fout, "\n\t\tSTART %s", t->sequence[i].start_value);

		if (t->sequence[i].minvalue)
			fprintf(fout, "\n\t\tMINVALUE %s", t->sequence[i].minvalue);
		else
			fprintf(fout, "\n\t\tNO MINVALUE");

		if (t->sequence[i].maxvalue)
			fprintf(fout, "\n\t\tMAXVALUE %s", t->sequence[i].maxvalue);
		else
			fprintf(fout, "\n\t\tNO MAXVALUE");


		if (strcmp(t->sequence[i].cycle_option, "NO") == 0)
			fprintf(fout, "\n\t\tNO CYCLE");
		else 
			fprintf(fout, "\n\t\tCYCLE");

		fprintf(fout, ";\n");


		owner = formatObjectIdentifier(t->sequence[i].relowner);
		fprintf(fout, "\n");
		fprintf(fout, "ALTER SEQUENCE %s.%s OWNER TO %s;\n", t->sequence[i].nspname, t->sequence[i].relname, owner);
		free(owner);

		/*generate acl on view*/
		dumpGrant(fout, OBSequence, t->sequence[i].nspname, t->sequence[i].relname, t->sequence[i].relacl, NULL);

		if (t->sequence[i].comment) {
			fprintf(fout, "\n");
			fprintf(fout, "COMMENT ON SEQUENCE %s.%s IS %s;\n",
					t->sequence[i].nspname, t->sequence[i].relname, t->sequence[i].comment);
		}

		fprintf(fout, "\n\n");
	}
}
