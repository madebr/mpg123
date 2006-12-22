#include "icy.h"
#include <stdlib.h>

struct icy_meta icy;

void init_icy()
{
	init_stringbuf(&icy.name);
	init_stringbuf(&icy.url);
	icy.data = NULL;
	icy.interval = 0;
	icy.next = 0;
	icy.changed = 0;
}

void clear_icy()
{
	/* if pointers are non-null, they have some memory */
	free_stringbuf(&icy.name);
	free_stringbuf(&icy.url);
	free(icy.data);
	init_icy();
}

void set_data(char* new_data)
{
	if(icy.data) free(icy.data);
	icy.data = new_data;
	icy.changed = 1;
}
