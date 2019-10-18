#ifndef TABLE_H
#define TABLE_H

#include "common.h"

typedef struct PGAttribute
{
	int			attnum;
	char		*attname;
	bool		attnotnull;
	char		*atttypname;
	char		*attdefexpr;
	char		*attcollation;
	int			attstattarget;
	char		*attstorage;
	bool		defstorage;
	char		*attoptions;
	char		*comment;
	char		*acl;
} PGAttribute;

typedef struct PGTable
{
	int oid;
	char *schema;
	char *table;
	int nattributes;
	PGAttribute *attributes;
} PGTable;

PGTable * getTable(PGconn *c, PGROption *opts);

#endif