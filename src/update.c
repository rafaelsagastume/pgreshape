#include "update.h"

void dumpUpdateData(FILE *fout, PGTable *t, PGROption *opts) {

	int offset_column = getAttnumOffset(t, opts);

	int i;


	fprintf(fout, "\nUPDATE %s.%s n SET \n", t->schema, t->table);

	for (i = 0; i < t->nattributes; ++i)
	{
		if (t->attributes[i].attnum > offset_column)
		{
			fprintf(fout, "\t%s = a.%s", t->attributes[i].attname, t->attributes[i].attname);
			if (i < t->nattributes-1)
			{
				fprintf(fout, ",\n");
			}
		}
	}

	fprintf(fout, "\nFROM (SELECT * FROM %s.%s_reshape_bk) a", t->schema, t->table);
	fprintf(fout, "\nWHERE (%s) = (%s);", t->primary_keys_aa, t->primary_keys_nn);

	fprintf(fout, "\n\nDROP TABLE %s.%s_reshape_bk;\n", t->schema, t->table);
}
