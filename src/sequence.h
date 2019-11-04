#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "common.h"
#include "acl.h"
#include "table.h"

void getSequences(PGconn *c, PGTable *t, PGROption *opts);
void dumpCreateSequences(FILE *fout, PGTable *t);

#endif
