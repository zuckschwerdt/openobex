#define IRCP_DEBUG 4
#ifdef IRCP_DEBUG
#define DEBUG(n, args...) if(n >= IRCP_DEBUG) g_print(args)
#else
#define DEBUG(n, ...)
#endif //IRCP_DEBUG[A

