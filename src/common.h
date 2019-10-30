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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

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

typedef struct PGCheckConstraint
{
	char *conname;
	char *condef;
} PGCheckConstraint;

typedef struct PGUnique
{
	char contype;
	char *schema;
	char *table;
	char *unique_name;
	char *columns;
	char *comment;
} PGUnique;

typedef struct PGIndex
{
	char contype;
	char *schema;
	char *relname;
	char *indexdef;
	char *comment;
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
	char *reloptions;
	char *checkoption;
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
	char *primary_keys;
	char *primary_keys_aa;
	char *primary_keys_nn;
	int ncheck;
	PGCheckConstraint *checks;
} PGTable;

typedef struct PGROption {

	char *schema;
	char *table;
	char *offset;
	char *column;
	char *type;

} PGROption;

char * formatObjectIdentifier(char *s);

#endif