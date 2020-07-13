dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.in)
AC_CONFIG_MACRO_DIRS([../m4 ../.. ../../config])

AC_CHECK_SIZEOF([size_t])
bitsize=`expr $ac_cv_sizeof_size_t \* 8`
SIM_AC_OPTION_BITSIZE([$bitsize])

SIM_AC_OUTPUT
