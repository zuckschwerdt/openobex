
#ifndef VISIBILITY_H
#define VISIBILITY_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(_WIN32)
#  include <winsock2.h> /* make sure to use version 2 of Windows socket API */
#  include <windows.h>
#  if defined(OPENOBEX_EXPORTS) || defined(DLL_EXPORT)
#    define LIB_SYMBOL __declspec(dllexport)
#  endif
#  define CALLAPI WINAPI
#elif defined(HAVE_VISIBILITY)
#  define LIB_SYMBOL __attribute__ ((visibility("default")))
#endif

#ifndef LIB_SYMBOL
#define LIB_SYMBOL
#endif

#ifndef CALLAPI
#define CALLAPI
#endif

#endif
