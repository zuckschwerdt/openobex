#ifndef OBEX_TEST_H
#define OBEX_TEST_H

#ifndef _WIN32
#include "obex_test_cable.h"
#endif


struct context
{
	int serverdone;
	int clientdone;
	char *get_name;	/* Name of last get-request */
};

#endif
