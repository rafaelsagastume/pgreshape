/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#include "acl.h"

char * formatAcl(char *s, char *c) {
	char	*acl_string, *ptr;
	size_t	ln_size;
	bool	first = true;
	int		i;
	size_t	len_col = 0;
	size_t	len_colto = 0;

	if (s == NULL){
		return NULL;
	}

	ln_size = strlen(s);
	if (c) {
		len_col = strlen(c);
		len_colto = len_col + 3;
	}

	acl_string = (char *) malloc (((ln_size * 20 + (ln_size - 1) * 2) + len_colto) * sizeof(char));
	acl_string[0] = '\0'; /*final char string*/
	ptr = acl_string;

	for (i = 0; i < ln_size; i++)
	{
		if (first)
			first = false;
		else
		{
			strncpy(ptr, ", ", 2);
			ptr += 2;
		}

		switch (s[i])
		{
			case 'r':
				strncpy(ptr, "SELECT", 6);
				ptr += 6;
				break;
			case 'U':
				strncpy(ptr, "USAGE", 5);
				ptr += 5;
				break;
			case 'a':
				strncpy(ptr, "INSERT", 6);
				ptr += 6;
				break;
			case 'x':
				strncpy(ptr, "REFERENCES", 10);
				ptr += 10;
				break;
			case 'd':
				strncpy(ptr, "DELETE", 6);
				ptr += 6;
				break;
			case 't':
				strncpy(ptr, "TRIGGER", 7);
				ptr += 7;
				break;
			case 'D':
				strncpy(ptr, "TRUNCATE", 8);
				ptr += 8;
				break;
			case 'w':
				strncpy(ptr, "UPDATE", 6);
				ptr += 6;
				break;
			case 'X':
				strncpy(ptr, "EXECUTE", 7);
				ptr += 7;
				break;
			case 'C':
				strncpy(ptr, "CREATE", 6);
				ptr += 6;
				break;
			case 'c':
				strncpy(ptr, "CONNECT", 7);
				ptr += 7;
				break;
			case 'T':
				strncpy(ptr, "TEMPORARY", 9);
				ptr += 9;
				break;
		}

		if (len_col > 0)
		{
			strncpy(ptr, " (", 2);
			ptr += 2;
			strncpy(ptr, c, len_col);
			ptr += len_col;
			strncpy(ptr, ")", 1);
			ptr += 1;
		}
	}

	*ptr = '\0';
	return acl_string;
}


Acl * splitAcl(char *a) {
	Acl	*acl_item;
	char	*ptr;
	char	*next;
	size_t	len;

	if (a == NULL)
		return NULL;

	acl_item = (Acl *) malloc(sizeof(Acl));
	acl_item->next = NULL;

	ptr = a;

	next = strchr(ptr, '=');
	if (next)
		*next++ = '\0';
	len = strlen(ptr);

	if (len == 0)
	{
		acl_item->grant = (char *) malloc(7 * sizeof(char));
		strcpy(acl_item->grant, "PUBLIC");
		acl_item->grant[6] = '\0';
	}
	else
	{
		acl_item->grant = (char *) malloc((len + 1) * sizeof(char));
		strncpy(acl_item->grant, ptr, len + 1);
	}

	ptr = next;

	next = strchr(ptr, '/');
	if (next)
		*next++ = '\0';
	len = strlen(ptr);
	acl_item->privileges = (char *) malloc((len + 1) * sizeof(char));
	strncpy(acl_item->privileges, ptr, len + 1);

	ptr = next;

	return acl_item;
}

void freeAclItem(Acl *item) {
	free(item->grant);
	free(item->privileges);
	free(item);
}

List * shapeACL(char *acl) {
	List	*acl_list;
	char	*tmp;
	char	*p;

	char	*item;
	char	*nextitem;
	size_t	len;

	if (acl == NULL)
	{
		return NULL;
	}

	len = strlen(acl);

	tmp = strdup(acl);
	p = tmp;


	if (tmp[len - 1] != '}')
	{
		free(tmp);
		return NULL;
	}

	p[len - 1] = '\0';

	if (tmp[0] != '{')
	{
		free(tmp);
		return NULL;
	}
	p++;

	acl_list = (List *) malloc(sizeof(List));
	acl_list->root = acl_list->leaf = NULL;

	for (item = p; item; item = nextitem)
	{
		Acl	*acl_item;

		nextitem = strchr(item, ',');
		if (nextitem)
			*nextitem++ = '\0';

		acl_item = splitAcl(item);

		/* order */
		if (acl_list->leaf)
		{
			Acl	*cur = acl_list->root;

			if (strcmp(cur->grant, acl_item->grant) > 0)
			{
				acl_item->next = cur;
				acl_list->root = acl_item;
			}
			else
			{
				while (cur != NULL)
				{
					if (cur == acl_list->leaf)
					{
						cur->next = acl_item;
						acl_list->leaf = acl_item;
						break;
					}

					if (strcmp(cur->grant, acl_item->grant) < 0 &&
							strcmp(cur->next->grant, acl_item->grant) >= 0)
					{
						acl_item->next = cur->next;
						cur->next = acl_item;
						break;
					}

					cur = cur->next;
				}
			}
		}
		else
		{
			acl_list->root = acl_item;
			acl_list->leaf = acl_item;
		}
	}

	free(tmp);
	return acl_list;
}

void freeAclList(List *acl_list) {
	Acl	*item, *tmp;

	if (acl_list == NULL)
		return;

	item = acl_list->root;
	tmp = item->next;
	while (item)
	{
		freeAclItem(item);
		item = tmp;
		if (tmp)
			tmp = tmp->next;
	}
	free(acl_list);
}


void dumpAcl(FILE *fout, int obj, char *sch, char *name, char *privs, char *grant, char *cols) {
	char *schema;
	char *objname;
	char *owner;
	char *p;

	if (privs == NULL)
		return;

	schema = NULL;
	objname = formatObjectIdentifier(name);

	if (strcmp(grant, "PUBLIC") == 0)
		owner = strdup(grant);
	else
		owner = formatObjectIdentifier(grant);

	p = formatAcl(privs, cols);

	fprintf(fout, "\n");
	fprintf(fout, "GRANT %s", p);
	fprintf(fout, " ON ");

	switch (obj)
	{
		case OBTable:
			fprintf(fout, "TABLE");
			break;
		case OBSequence:
			fprintf(fout, "SEQUENCE");
			break;
	}

	if (obj == OBTable || obj == OBSequence) {
		schema = formatObjectIdentifier(sch);

		fprintf(fout, " %s.%s TO %s;\n",
				schema,
				objname,
				owner);
	}

	free(p);
	free(objname);
	if (schema)
		free(schema);
	free(owner);
}


void dumpGrant(FILE *fout, int obj, char *sch, char *name, char *acla, char *cols) {
	List *ala;
	Acl	*tmpa = NULL;

	ala = shapeACL(acla);

	if (ala)
		tmpa = ala->root;

	while (tmpa != NULL)
	{
		dumpAcl(fout, obj, sch, name, tmpa->privileges, tmpa->grant, cols);
		tmpa = tmpa->next;
	}

	freeAclList(ala);
}
