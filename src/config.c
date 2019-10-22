/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */
#include "config.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PGR_READ_BUF 1000

char *trim(char *str);

char *trim(char *str)
{
	char *end;

	while(isspace((unsigned char)*str)) 
		str++;

	if(*str == 0)
		return str;

	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end))
		end--;

	*(end+1) = 0;

	return str;
}


ConfigFile *loadConfig(const char *path)
{
	
	ConfigFile *conf = NULL;
	FILE *file = fopen(path, "ro");
	char *buf, *p, *pp, *key, *val;

	if(file) {

		conf = calloc(1, sizeof(ConfigFile));
		buf = malloc(PGR_READ_BUF);

		while(!feof(file) && fgets(buf, PGR_READ_BUF, file)!=NULL) {

			if(buf[0] == '\n')
				continue;
			if(buf[0] == '#')
				continue;

			pp = buf;
			p = strsep(&pp, "=");
			key = trim(p);
			if(pp)
				val = trim(pp);
			else
				val = NULL;


			if(strcmp(key, "host") == 0)
				conf->host = strdup(val);
			if(strcmp(key, "port") == 0)
				conf->port = strdup(val);
			if(strcmp(key, "dbname") == 0)
				conf->dbname = strdup(val);
			if(strcmp(key, "user") == 0)
				conf->user = strdup(val);
			if(strcmp(key, "password") == 0)
				conf->password = strdup(val);
			if(strcmp(key, "file") == 0)
				conf->file = strdup(val);

		}

		fclose(file);

	} else {
		perror("fopen");
	}
	
	free(buf);
	
	return conf;
}

void freeConfig(ConfigFile *config)
{
	if(config) {
		
		free(config[0].host);
		free(config[0].port);
		free(config[0].user);
		free(config[0].password);
		free(config[0].dbname);
		free(config);
		
	}
}