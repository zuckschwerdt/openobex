//#define DEBUG_TCP 1

#if defined(_MSC_VER) && _MSC_VER < 1400
static void DEBUG(int n, char *format, ...) {}

#elif IRCP_DEBUG
#  define DEBUG(n, format, ...)						\
	if(n <= IRCP_DEBUG)						\
		fprintf(stderr, "%s(): " format, __FUNCTION__ , ## __VA_ARGS__)

#else
#  define DEBUG(n, format, ...)
#endif
