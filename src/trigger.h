#ifndef TRIGGER_H
#define TRIGGER_H

#include "common.h"

void dumpDisableTriggerAll(FILE *fout, PGTable *t);
void dumpEnableTriggerAll(FILE *fout, PGTable *t);

#endif