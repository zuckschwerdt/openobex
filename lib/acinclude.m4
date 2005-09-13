dnl
dnl Option to enable or disable IrDA support
dnl

AC_DEFUN([IRDA_CHECK],[
AC_ARG_ENABLE([irda],
              [AS_HELP_STRING([--disable-irda],
                              [Disables libopenobex irda support @<:@default=auto@:>@])],
              [ac_irda_enabled=$enableval], [ac_irda_enabled=yes])

if test "$ac_irda_enabled" = yes; then
	AC_CACHE_CHECK([for IrDA support], am_cv_irda_found,[
		AC_TRY_COMPILE([#include <sys/socket.h>
				#include "src/irda.h"],
				[struct irda_device_list l;],
				am_cv_irda_found=yes,
				am_cv_irda_found=no)])
	if test $am_cv_irda_found = yes; then
		AC_DEFINE([HAVE_IRDA], [1], [Define if system supports IrDA and it's enabled])
	fi
fi
])


dnl
dnl Option to enable or disable Bluetooth support
dnl

AC_DEFUN([BLUETOOTH_CHECK],[  
AC_ARG_ENABLE([bluetooth],
              [AS_HELP_STRING([--disable-bluetooth],
                              [Disables libopenobex bluetooth support @<:@default=auto@:>@])],
       	      [ac_bluetooth_enabled=$enableval], [ac_bluetooth_enabled=yes])

if test "$ac_bluetooth_enabled" = yes; then
	AC_CACHE_CHECK([for Bluetooth support], am_cv_bluetooth_found,[

		AC_TRY_COMPILE([#include <sys/socket.h>
				#include <bluetooth/bluetooth.h>
				#include <bluetooth/rfcomm.h>],
				[bdaddr_t bdaddr;
				struct sockaddr_rc addr;],
				am_cv_bluetooth_found=yes,
				am_cv_bluetooth_found=no)])
	if test $am_cv_bluetooth_found = yes; then
		AC_DEFINE([HAVE_BLUETOOTH], [1], [Define if system supports Bluetooth and it's enabled])
	fi
fi
])
