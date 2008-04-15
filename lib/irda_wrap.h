#ifndef IRDA_WRAP_H
#define IRDA_WRAP_H

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 1
#endif

#include <af_irda.h>
#define irda_device_info _WINDOWS_IRDA_DEVICE_INFO
#define irda_device_list _WINDOWS_DEVICELIST

#define sockaddr_irda _SOCKADDR_IRDA
#define sir_family irdaAddressFamily
#define sir_name   irdaServiceName

#else /* _WIN32 */

#include "irda.h"

#endif /* _WIN32 */

#endif /* IRDA_WRAP_H */
