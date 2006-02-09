AC_DEFUN([AC_PROG_CC_PIE], [
	AC_CACHE_CHECK([whether ${CC-cc} accepts -fPIE], ac_cv_prog_cc_pie, [
		echo 'void f(){}' > conftest.c
		if test -z "`${CC-cc} -fPIE -pie -c conftest.c 2>&1`"; then
			ac_cv_prog_cc_pie=yes
		else
			ac_cv_prog_cc_pie=no
		fi
		rm -rf conftest*
	])
])

AC_DEFUN([AC_INIT_OPENOBEX], [
	AC_PREFIX_DEFAULT(/usr/local)

	if (test "${CFLAGS}" = ""); then
		CFLAGS="-Wall -O2"
	fi

	if (test "${prefix}" = "NONE"); then
		dnl no prefix and no sysconfdir, so default to /etc
		if (test "$sysconfdir" = '${prefix}/etc'); then
			AC_SUBST([sysconfdir], ['/etc'])
		fi

		dnl no prefix and no mandir, so use ${prefix}/share/man as default
		if (test "$mandir" = '${prefix}/man'); then
			AC_SUBST([mandir], ['${prefix}/share/man'])
		fi

		prefix="${ac_default_prefix}"
	fi

	if (test "${libdir}" = '${exec_prefix}/lib'); then
		libdir="${prefix}/lib"
	fi

	if (test "$sysconfdir" = '${prefix}/etc'); then
		configdir="${prefix}/etc/openobex"
	else
		configdir="${sysconfdir}/openobex"
	fi

	AC_DEFINE_UNQUOTED(CONFIGDIR, "${configdir}", [Directory for the configuration files])
])

AC_DEFUN([AC_PATH_IRDA], [
	AC_CACHE_CHECK([for IrDA support], irda_found, [
		AC_TRY_COMPILE([
				#include <sys/socket.h>
				#include "lib/irda.h"
			], [
				struct irda_device_list l;
			], irda_found=yes, irda_found=no)
	])
])

AC_DEFUN([AC_PATH_BLUEZ], [
	bluez_prefix=${prefix}

	AC_ARG_WITH(bluez, AC_HELP_STRING([--with-bluez=DIR], [BlueZ library is installed in DIR]), [
		if (test "${withval}" != "yes"); then
			bluez_prefix=${withval}
		fi
	])

	ac_save_CPPFLAGS=$CPPFLAGS
	ac_save_LDFLAGS=$LDFLAGS

	BLUEZ_CFLAGS=""
	test -d "${bluez_prefix}/include" && BLUEZ_CFLAGS="$BLUEZ_CFLAGS -I${bluez_prefix}/include"

	CPPFLAGS="$CPPFLAGS $BLUEZ_CFLAGS"
	AC_CHECK_HEADER(bluetooth/bluetooth.h, bluez_found=yes, bluez_found=no)

	BLUEZ_LIBS=""
	if (test "${prefix}" = "${bluez_prefix}"); then
		test -d "${libdir}" && BLUEZ_LIBS="$BLUEZ_LIBS -L${libdir}"
	else
		test -d "${bluez_prefix}/lib64" && BLUEZ_LIBS="$BLUEZ_LIBS -L${bluez_prefix}/lib64"
		test -d "${bluez_prefix}/lib" && BLUEZ_LIBS="$BLUEZ_LIBS -L${bluez_prefix}/lib"
	fi

	LDFLAGS="$LDFLAGS $BLUEZ_LIBS"
	AC_CHECK_LIB(bluetooth, hci_open_dev, BLUEZ_LIBS="$BLUEZ_LIBS -lbluetooth", bluez_found=no)
	AC_CHECK_LIB(bluetooth, sdp_connect, dummy=yes, AC_CHECK_LIB(sdp, sdp_connect, BLUEZ_LIBS="$BLUEZ_LIBS -lsdp"))

	CPPFLAGS=$ac_save_CPPFLAGS
	LDFLAGS=$ac_save_LDFLAGS

	AC_SUBST(BLUEZ_CFLAGS)
	AC_SUBST(BLUEZ_LIBS)
])

AC_DEFUN([AC_PATH_USB], [
	usb_prefix=${prefix}

	AC_ARG_WITH(usb, AC_HELP_STRING([--with-usb=DIR], [USB library is installed in DIR]), [
		if (test "$withval" != "yes"); then
			usb_prefix=${withval}
		fi
	])

	ac_save_CPPFLAGS=$CPPFLAGS
	ac_save_LDFLAGS=$LDFLAGS

	USB_CFLAGS=""
	test -d "${usb_prefix}/include" && USB_CFLAGS="$USB_CFLAGS -I${usb_prefix}/include"

	CPPFLAGS="$CPPFLAGS $USB_CFLAGS"
	AC_CHECK_HEADER(usb.h, usb_found=yes, usb_found=no)

	USB_LIBS=""
	if (test "${prefix}" = "${usb_prefix}"); then
		test -d "${libdir}" && USB_LIBS="$USB_LIBS -L${libdir}"
	else
		test -d "${usb_prefix}/lib64" && USB_LIBS="$USB_LIBS -L${usb_prefix}/lib64"
		test -d "${usb_prefix}/lib" && USB_LIBS="$USB_LIBS -L${usb_prefix}/lib"
	fi

	LDFLAGS="$LDFLAGS $USB_LIBS"
	AC_CHECK_LIB(usb, usb_open, USB_LIBS="$USB_LIBS -lusb", usb_found=no)
	AC_CHECK_LIB(usb, usb_get_busses, dummy=yes, AC_DEFINE(NEED_USB_GET_BUSSES, 1, [Define to 1 if you need the usb_get_busses() function.]))
	AC_CHECK_LIB(usb, usb_interrupt_read, dummy=yes, AC_DEFINE(NEED_USB_INTERRUPT_READ, 1, [Define to 1 if you need the usb_interrupt_read() function.]))

	case "${host_cpu}-${host_os}" in
		*-darwin*)
			USB_LIBS="$USB_LIBS -framework CoreFoundation -framework IOKit"
		;;
	esac

	CPPFLAGS=$ac_save_CPPFLAGS
	LDFLAGS=$ac_save_LDFLAGS

	AC_SUBST(USB_CFLAGS)
	AC_SUBST(USB_LIBS)
])

AC_DEFUN([AC_ARG_OPENOBEX], [
	fortify_enable=yes
	irda_enable=yes
	bluetooth_enable=yes
	usb_enable=yes
	apps_enable=no
	debug_enable=no
	syslog_enable=no
	dump_enable=no

	AC_ARG_ENABLE(fortify, AC_HELP_STRING([--disable-fortify], [disable compile time buffer checks]), [
		fortify_enable=${enableval}
	])

	AC_ARG_ENABLE(irda, AC_HELP_STRING([--disable-irda], [disable IrDA support]), [
		irda_enable=${enableval}
	])

	AC_ARG_ENABLE(bluetooth, AC_HELP_STRING([--disable-bluetooth], [disable Bluetooth support]), [
		bluetooth_enable=${enableval}
	])

	AC_ARG_ENABLE(usb, AC_HELP_STRING([--disable-usb], [disable USB support]), [
		usb_enable=${enableval}
	])

	AC_ARG_ENABLE(apps, AC_HELP_STRING([--enable-apps], [enable test applications]), [
		apps_enable=${enableval}
	])

	AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [enable compiling with debugging information]), [
		debug_enable=${enableval}
	])

	AC_ARG_ENABLE(syslog, AC_HELP_STRING([--enable-syslog], [enable debugging to the system logger]), [
		syslog_enable=${enableval}
	])

	AC_ARG_ENABLE(dump, AC_HELP_STRING([--enable-dump], [enable protocol dumping for debugging]), [
		dump_enable=${enableval}
	])

	if (test "${fortify_enable}" = "yes"); then
		CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2"
	fi

	REQUIRES=""

	if (test "${irda_enable}" = "yes" && test "${irda_found}" = "yes"); then
		AC_DEFINE(HAVE_IRDA, 1, [Define if system supports IrDA and it's enabled])
	fi

	if (test "${bluetooth_enable}" = "yes" && test "${bluez_found}" = "yes"); then
		AC_DEFINE(HAVE_BLUETOOTH, 1, [Define if system supports Bluetooth and it's enabled])
		REQUIRES="bluez"
	fi

	if (test "${usb_enable}" = "yes" && test "${usb_found}" = "yes"); then
		AC_DEFINE(HAVE_USB, 1, [Define if system supports USB and it's enabled])
		AC_CHECK_FILE(${usb_prefix}/lib/pkgconfig/libusb.pc, REQUIRES="$REQUIRES libusb")
	fi

	AM_CONDITIONAL(APPS, test "${apps_enable}" = "yes")

	if (test "${debug_enable}" = "yes" && test "${ac_cv_prog_cc_g}" = "yes"); then
		CFLAGS="$CFLAGS -g"
		AC_DEFINE_UNQUOTED(OBEX_DEBUG, 1, [Enable debuggging])
	fi

	if (test "${syslog_enable}" = "yes"); then
		AC_DEFINE_UNQUOTED(OBEX_SYSLOG, 1, [System logger debugging])
	fi

	if (test "${dump_enable}" = "yes"); then
		AC_DEFINE_UNQUOTED(OBEX_DUMP, 1, [Protocol dumping])
	fi

	AC_SUBST(REQUIRES)
])
