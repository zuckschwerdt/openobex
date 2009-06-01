#ifndef OBEX_INCL_H
#define OBEX_INCL_H

#include "bluez_compat.h"
#include "visibility.h"

/* This overides the define in openobex/obex.h */
#define OPENOBEX_SYMBOL(retval) LIB_SYMBOL retval CALLAPI
#include <openobex/obex.h>

#endif
