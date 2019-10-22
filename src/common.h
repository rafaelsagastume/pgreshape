/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <c.h>
typedef struct PGForeignKey
{
	char *conname;
	char *condef;
} PGForeignKey;

typedef struct PGUnique
{
	char contype;
	char *relname;
	char *pg_get_indexdef;
} PGUnique;

typedef struct PGIndex
{
	char contype;
	char *relname;
	char *pg_get_indexdef;
} PGIndex;

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

typedef struct PGView
{
	int oid;
	char *schema;
	char *view;
	char relkind;
	char *view_definition;
	char *relowner;
	char *comment;
	char *acl;
} PGView;

typedef struct PGTable
{
	int oid;
	char *schema;
	char *table;
	int nattributes;
	PGAttribute *attributes;
	int nindexes;
	PGIndex *indexes;
	int nunique;
	PGUnique *unique;
	int nforeignkey;
	PGForeignKey *foreignkeys;
	int nviews;
	PGView *views;
} PGTable;

typedef struct PGROption {

	char *schema;
	char *table;
	char *offset;
	char *column;
	char *type;

} PGROption;

#endif