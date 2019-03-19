#! /bin/sh

# local options:  ac_help is the help message that describes them
# and LOCAL_AC_OPTIONS is the script that interprets them.  LOCAL_AC_OPTIONS
# is a script that's processed with eval, so you need to be very careful to
# make certain that what you quote is what you want to quote.

ac_help="
--use-termcap		Link with termcap instead of curses, if possible
--partial-install	Don\'t install the lv, lv(1) name links
--size=NNN		Use a NNN-byte edit buffer
--stdio			Prefer stdio for i/o
--tputs			Use tputs() to slow down tty output
--logging		Log tty output to levee.log
--noglob		Don't use system glob() even if it exists
--windows		compile for windows 8+
--dos			compile for ms-dos or old versions of microsoft windows
--tos			compile for the Atari ST
--rmx			compile for RMX
--os2			compiler for OS/2
--flexos		compile for FlexOS"

LOCAL_AC_OPTIONS='
case Z$1 in
Z--partial-install)
	    missing_lv=1;;
Z--dos)	    ac_os=DOS;;
Z--tos)     ac_os=ATARI=1;;
Z--flexos)  ac_os=FLEXOS=1;;
Z--rmx)	    ac_os=RMX;;
Z--win10)   ac_os=WINDOWS;;
Z--size=*)  SIZE=$(echo Z$1 | sed -e 's/^Z--size=//') ;;
Z--stdio)   USING_STDIO=1;;
Z--tputs)   USING_TPUTS=1;;
Z--logging) USING_LOGGING=1;;
Z--noglob)  NO_GLOB=1;;
*)          ac_error=1;;
esac;shift'

# load in the configuration file
#
TARGET=levee
. ./configure.inc
AC_INIT $TARGET

# validate --size=
#
case X"${SIZE}" in
X[0-9][0-9]*)	 ;;
X[0-9][0-9]*[Ll]);;
X)		 ;;
X*)		 AC_FAIL "--size=$SIZE is not a valid number" ;;
esac

AC_PROG_CC
unset _MK_LIBRARIAN

if [ -n "$USING_LOGGING" ]; then
    AC_SUB LOGGER	logit.o
    AC_DEFINE LOGGING	1
else
    AC_SUB LOGGER	''
fi

test -n "$USING_STDIO" && AC_DEFINE USING_STDIO 1
test -n "$USING_TPUTS" && AC_DEFINE USING_TPUTS 1

if [ "$OS_DOS" ]; then
    AC_DEFINE	EDITSIZE ${SIZE:-32000}
    AC_DEFINE	OS_DOS	1
    AC_SUB	MACHDEP	dos
elif [ "$OS_OS2" ]; then
    AC_DEFINE	EDITSIZE ${SIZE:-32000}
    AC_DEFINE	OS_OS2 1
    AC_SUB	MACHDEP os2
elif [ "$OS_ATARI" ]; then
    AC_DEFINE	EDITSIZE ${SIZE:-32000}
    AC_DEFINE	OS_ATARI	1
    AC_SUB	MACHDEP	gem
    AC_DEFINE	HAVE_BLKFILL	1
elif [ "$OS_FLEXOS" ]; then
    AC_DEFINE	EDITSIZE ${SIZE:-256000}
    AC_DEFINE	OS_FLEXOS	1
    AC_SUB	MACHDEP	flex
elif [ "$OS_WINDOWS" ]; then
    AC_DEFINE   EDITSIZE ${SIZE:-256000}
    AC_DEFINE	OS_WINDOWS	1
    AC_SUB	MACHDEP	win
else
    AC_DEFINE	EDITSIZE ${SIZE:-256000}
    AC_DEFINE	OS_UNIX	1
    AC_SUB	MACHDEP	unix

    if AC_CHECK_HEADERS string.h; then
	# Assume a mainly ANSI-compliant world, where the
	# existance of string.h implies a memset() and strchr()
	AC_DEFINE HAVE_MEMSET	1
	AC_DEFINE HAVE_STRCHR	1
	AC_DEFINE HAVE_STRDUP	1
    else
	AC_CHECK_FUNCS memset
	AC_CHECK_FUNCS strchr
	AC_CHECK_FUNCS strdup
    fi

    if AC_CHECK_HEADERS signal.h; then
	# Assume a mainly sane world where the existance
	# of signal.h means that signal() exists
	AC_DEFINE HAVE_SIGNAL 1
    fi

    if [ "$USE_TERMCAP" ]; then
	LIBORDER="-ltermcap -lcurses -lncurses"
    else
	LIBORDER="-lcurses -lncurses -ltermcap"
    fi

    AC_CHECK_HEADERS termcap.h || AC_FAIL "levee needs <termcap.h>"

    if AC_LIBRARY tgetent $LIBORDER; then
	AC_DEFINE USE_TERMCAP	1
    else
	# have to use a local termcap
	AC_DEFINE TERMCAP_EMULATION	1
	AC_DEFINE USE_TERMCAP	1
    fi

    AC_CHECK_HEADERS termios.h && AC_CHECK_FUNCS tcgetattr
fi

AC_CHECK_FUNCS basename && AC_CHECK_HEADERS libgen.h

if [ "$NO_GLOB" ]; then
    TLOG "Using builtin glob()"
elif AC_CHECK_HEADERS glob.h; then
    if AC_CHECK_FUNCS 'glob((char*)0, GLOB_NOMAGIC, (void*)0, (void*)0)' glob.h; then
	AC_DEFINE 'USING_GLOB' 1
    elif AC_CHECK_FUNCS glob; then
	TLOG "glob() exists, but doesn't support GLOB_NOMAGIC"
    fi
fi

if AC_PROG_LN_S && test -z "$missing_lv"; then
    AC_SUB NOMK ''
else
    AC_SUB NOMK '@#'
fi

AC_OUTPUT Makefile
