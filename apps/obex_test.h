#ifndef OBEX_TEST_H
#define OBEX_TEST_H

#ifndef _WIN32
#include "obex_test_cable.h"
#endif


struct context
{
	gboolean serverdone;
	gboolean clientdone;
};

#endif
