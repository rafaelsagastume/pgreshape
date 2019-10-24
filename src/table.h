#ifndef TABLE_H
#define TABLE_H

#include "common.h"

PGTable * getTable(PGconn *c, PGROption *opts);
void getTableAttributes(PGconn *c, PGTable *t);
void getTableIndexes(PGconn *c, PGTable *t);
void getTableUnique(PGconn *c, PGTable *t);
void getTableForeignKey(PGconn *c, PGTable *t);
void dumpDropIndex(FILE *fout, PGTable *t);
void dumpDropUnique(FILE *fout, PGTable *t);
void dumpDropForeignKey(FILE *fout, PGTable *t);
void dumpCreateTempTableBackup(FILE *fout, PGTable *t);
void dumpDropTableColumn(FILE *fout, PGTable *t, PGROption *opts);
int getAttnumOffset(PGTable *t, PGROption *opts);
void dumpNewColumn(FILE *fout, PGTable *t, PGROption *opts);
void dumpColumnTable(FILE *fout, PGTable *t, PGROption *opts);
void dumpCreateForeignKey(FILE *fout, PGTable *t);
void dumpCreateIndex(FILE *fout, PGTable *t);
void dumpCreateUnique(FILE *fout, PGTable *t);
int existsColumn(PGTable *t, char *column);
void dumpSetNotNullColumnTable(FILE *fout, PGTable *t, PGROption *opts);

#endif