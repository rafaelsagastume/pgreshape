/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#include "schema.h"

int existsSchema(PGconn *c, char *schema)
{
	PGresult *res;
	int n;
	char *query = NULL;

	asprintf(&query,
		"select schema_name from information_schema.schemata where schema_name = '%s';", schema);
	
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
