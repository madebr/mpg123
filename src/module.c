/*
	module.c: modular code loader

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas J Humfrey
*/

#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <ltdl.h>

#include "mpg123app.h"

#ifndef HAVE_LTDL
#error Cannot build without LTDL library support
#endif

#define MODULE_FILE_SUFFIX		".la"
#define MODULE_SYMBOL_PREFIX 	"mpg123_"
#define MODULE_SYMBOL_SUFFIX 	"_module_info"


/* Open a module */
mpg123_module_t*
open_module( const char* type, const char* name )
{
	lt_dlhandle handle = NULL;
	mpg123_module_t *module = NULL;
	char* module_name = strdup( name );
	char* module_path = NULL;
	int module_path_len = 0;
	char* module_symbol = NULL;
	int module_symbol_len = 0;
	int i;

	/* Initialize libltdl */
	if (lt_dlinit()) error( "Failed to initialise libltdl" );
	
	/* Clean up the module name to prevent loading random files */
	for(i=0; i<strlen(module_name); i++) {
		if (!isalnum(module_name[i])) module_name[i] = '_';
	}

	/* Work out the path of the module to open */
	module_path_len = strlen( PKGLIBDIR ) + 1 + 
					  strlen( type ) + 1 + strlen( module_name ) +
					  strlen( MODULE_FILE_SUFFIX ) + 1;
	module_path = malloc( module_path_len );
	if (module_path == NULL) {
		error1( "Failed to allocate memory for module name: %s", strerror(errno) );
		return NULL;
	}
	snprintf( module_path, module_path_len, "%s/%s_%s%s", PKGLIBDIR, type, module_name, MODULE_FILE_SUFFIX );
	
	
	/* Display the path of the module created */
	debug1( "Module path: %s", module_path );


	/* Open the module */
	handle = lt_dlopen( module_path );
	free( module_path );
	free( module_name );
	if (handle==NULL) {
		error1( "Failed to open module: %s", lt_dlerror() );
		return NULL;
	}
	
	/* Work out the symbol name */
	module_symbol_len = strlen( MODULE_SYMBOL_PREFIX ) +
						strlen( type )  +
						strlen( MODULE_SYMBOL_SUFFIX ) + 1;
	module_symbol = malloc( module_path_len );
	snprintf( module_symbol, module_symbol_len, "%s%s%s", MODULE_SYMBOL_PREFIX, type, MODULE_SYMBOL_SUFFIX );
	debug1( "Module symbol: %s", module_symbol );
	
	/* Get the information structure from the module */
	module = (mpg123_module_t*)lt_dlsym(handle, module_symbol );
	if (module==NULL) {
		error1( "Failed to get module symbol: %s", lt_dlerror() );
		lt_dlclose( handle );
		return NULL;
	}
	free( module_symbol );
	
	/* Check the API version */
	if (MPG123_MODULE_API_VERSION > module->api_version) {
		error( "API version of module is too old" );
		lt_dlclose( handle );
		return NULL;
	} else if (MPG123_MODULE_API_VERSION > module->api_version) {
		error( "API version of module is too new" );
		lt_dlclose( handle );
		return NULL;
	}

	/* Store handle in the data structure */
	module->handle = handle;

	return module;
}


void close_module( mpg123_module_t* module )
{
	lt_dlhandle handle = module->handle;
	int err = lt_dlclose( handle );
	
	if (err) error1("Failed to close module: %s", lt_dlerror() );

}


void list_modules()
{
	DIR* dir = NULL;
	struct dirent *dp = NULL;

	
	/* Open the module directory */
	dir = opendir( PKGLIBDIR );
	if (dir==NULL) {
		error2("Failed to open the module directory (%s): %s\n", PKGLIBDIR, strerror(errno));
		exit(-1);
	}
	
	/* Display the program title */
	/* print_title(stderr); */

	/* List the output modules */
	printf("\n");
	printf("Available modules\n");
	printf("-----------------\n");
	
	while( (dp = readdir(dir)) != NULL ) {
		if (dp->d_type == DT_REG) {
			char* ext = dp->d_name + strlen( dp->d_name ) - strlen( MODULE_FILE_SUFFIX );
			if (strcmp(ext, MODULE_FILE_SUFFIX) == 0)
			{
				char *module_name = NULL;
				char *module_type = NULL;
				char *uscore_pos = NULL;
				mpg123_module_t *module = NULL;
				
				/* Extract the module type */
				module_type = strdup( dp->d_name );
				uscore_pos = strchr( module_type, '_' );
				if (uscore_pos==NULL) continue;
				if (uscore_pos>=module_type+strlen(module_type)+1) continue;
				*uscore_pos = '\0';
				
				/* Extract the short name of the module */
				module_name = strdup( dp->d_name + strlen( module_type ) + 1 );
				module_name[ strlen( module_name ) - strlen( MODULE_FILE_SUFFIX ) ] = '\0';
				
				/* Open the module */
				module = open_module( module_type, module_name );
				if (module) {
					printf("%-15s%s  %s\n", module->name, module_type, module->description );
				
					/* Close the module again */
					close_module( module );
				}
				
				free( module_name );
			}
		}
	}

	closedir( dir );
	
	exit(0);
}


