#ifndef OPENOBEX_DEBUG_H
#define OPENOBEX_DEBUG_H

#if defined(_MSC_VER) && _MSC_VER < 1400
static void log_debug(char *format, ...) {}
#define log_debug_prefix ""

#elif defined(OBEX_SYSLOG) && !defined(_WIN32)
#include <syslog.h>
#define log_debug(format, ...) syslog(LOG_DEBUG, format, ## __VA_ARGS__)
#define log_debug_prefix "OpenOBEX: "

#else
#include <stdio.h>
#define log_debug(format, ...) fprintf(stderr, format, ## __VA_ARGS__)
#define log_debug_prefix ""
#endif

/* use integer:  0 for production
 *               1 for verification
 *              >2 for debug
 */
#if defined(_MSC_VER) && _MSC_VER < 1400
static void DEBUG(int n, char *format, ...) {}

#elif OBEX_DEBUG
extern int obex_debug;
#  define DEBUG(n, format, ...) \
          if (obex_debug >= (n)) \
            log_debug("%s%s(): " format, log_debug_prefix, __FUNCTION__, ## __VA_ARGS__)

#else
#  define DEBUG(n, format, ...)
#endif


/* use bitmask: 0x1 for sendbuff
 *              0x2 for receivebuff
 */
#if OBEX_DUMP
extern int obex_dump;
#define DUMPBUFFER(n, label, msg) \
        if ((obex_dump & 0x3) & (n)) buf_dump(msg, label);
#else
#define DUMPBUFFER(n, label, msg)
#endif

#endif
