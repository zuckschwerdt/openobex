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

dnl
dnl USB_HOOK (script-if-usb-found, failflag)
dnl
dnl if failflag is "failure" it aborts if obex is not found.
dnl

AC_DEFUN([USB_HOOK],[
	AC_CACHE_CHECK([for USB support],am_cv_usb_found,[

		AC_TRY_COMPILE([#include <usb.h>],
		[struct usb_dev_handle *dev;],
		am_cv_usb_found=yes,
		am_cv_usb_found=no)])

		if test $am_cv_usb_found = yes; then
			AC_DEFINE(HAVE_USB,1,[Define if system supports USB])
			USB_LIBS="-lusb"
		fi
		AC_SUBST(USB_LIBS)
	])

])

AC_DEFUN([USB_CHECK], [
	USB_HOOK([],failure)
])
