#include "icy.h"
#include <stdlib.h>

void init_icy(struct icy_meta *icy)
{
	icy->data = NULL;
}

void clear_icy(struct icy_meta *icy)
{
	if(icy->data != NULL) free(icy->data);
	init_icy(icy);
}

void reset_icy(struct icy_meta *icy)
{
	clear_icy(icy);
	init_icy(icy);
}
/*void set_icy(struct icy_meta *icy, char* new_data)
{
	if(icy->data) free(icy->data);
	icy->data = new_data;
	icy->changed = 1;
}*/
