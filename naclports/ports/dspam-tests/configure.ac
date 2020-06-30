#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dspam-tests], [1.0], [ian.glen.neal@utexas.edu])
AC_CONFIG_SRCDIR([util.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldspam':
AC_CHECK_LIB([dspam], [main])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([Makefile])

### Human added arguments

# Add a few arguments to make running tests as easy as possible
AC_ARG_VAR(DSPAM_INSTALL, Location of the dspam install directory)

AC_ARG_VAR(DATA_DIR, Location where the test data should live.)
# Default
AS_IF([test "x$DATA_DIR" = x], [DATA_DIR="./etc"])

### End human modifications

AC_OUTPUT
