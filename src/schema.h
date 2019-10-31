/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include "common.h"

int existsSchema(PGconn *c, char *schema);

#endif
