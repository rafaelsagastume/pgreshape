/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    
    char *host;
    char *port;
    char *user;
    char *password;
    char *dbname;
    
} ConfigFile;

ConfigFile *loadConfig(const char *path);
void freeConfig(ConfigFile *config);

#endif 

