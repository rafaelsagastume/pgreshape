#include "trigger.h"

void dumpDisableTriggerAll(FILE *fout, PGTable *t) {

	fprintf(fout, "ALTER TABLE %s.%s DISABLE TRIGGER ALL;\n", t->schema, t->table);
}


void dumpEnableTriggerAll(FILE *fout, PGTable *t) {

	fprintf(fout, "ALTER TABLE %s.%s ENABLE TRIGGER ALL;\n", t->schema, t->table);	
}