dnl
dnl IRDA_HOOK (script-if-obex-found, failflag)
dnl
dnl if failflag is "failure" it aborts if obex is not found.
dnl

AC_DEFUN([IRDA_HOOK],[
	AC_DEFINE(HAVE_IRDA,1,[Define if system supports IrDA])

#	AC_CACHE_CHECK([for IrDA support in kernel],am_cv_irda_found,[
#		AC_CHECK_HEADER(irda.h,
#			am_cv_irda_found=yes,
#			am_cv_irda_found=no)
#	])
#	if test $am_cv_irda_found = yes; then
#		AC_DEFINE(HAVE_IRDA,1,[Define if system supports IrDA])
#	else
#		AC_MSG_RESULT(IrDA support not found)
#	fi
])

AC_DEFUN([IRDA_CHECK], [
	IRDA_HOOK([],failure)
])
