#ifndef VIEW_H
#define VIEW_H

#include "common.h"
#include "acl.h"

void getDependentViews(PGconn *c, PGTable *t);
void dumpDropDependentView(FILE *fout, PGTable *t);
void dumpCreateCreateView(FILE *fout, PGTable *t);
void getViewSecurityLabels(PGconn *c, PGTable *t);

#endif
