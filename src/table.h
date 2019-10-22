#ifndef TABLE_H
#define TABLE_H

#include "common.h"

PGTable * getTable(PGconn *c, PGROption *opts);
void getTableAttributes(PGconn *c, PGTable *t);
void getTableIndexes(PGconn *c, PGTable *t);
void getTableUnique(PGconn *c, PGTable *t);
void getTableForeignKey(PGconn *c, PGTable *t);

#endif