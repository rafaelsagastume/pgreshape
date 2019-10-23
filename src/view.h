#ifndef TABLE_H
#define TABLE_H

#include "common.h"

void getDependentViews(PGconn *c, PGTable *t);
void dumpCreateCreateView(FILE *fout, PGTable *t);

#endif