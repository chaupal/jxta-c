dnl
dnl LIBXML2_VERSION_TEST
dnl
dnl $1 = prefix location of libxml2 install
dnl
dnl Checks to see if the given version of libxml2
dnl is available and sets flags to indicate
dnl internal libxml2 api availability
dnl
AC_DEFUN([LIBXML2_VERSION_TEST], [

xml2_xpath_rewrite_version="2.6.26"
xml_version_valid="no"
xml_version_check="libxml-2.0 >= $xml2_xpath_rewrite_version"

dnl Sets up the pkg-config path in case the install location
dnl is non-standard
export PKG_CONFIG_PATH=$1/lib/pkgconfig:$PKG_CONFIG_PATH


dnl Check to make sure pkg-config is available
if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
fi

if test "$PKG_CONFIG" = "no" ; then
    echo "pkg-config script not found"
else
    AC_MSG_CHECKING(for $xml_version_check)
    if $PKG_CONFIG --exists "$xml_version_check" ; then
        AC_MSG_RESULT(yes)
        LIBXML2_XPATH_REWRITE_FLAG="-DLIBXML2_XPATH_REWRITE"
        AC_SUBST(LIBXML2_XPATH_REWRITE_FLAG)
    else
        AC_MSG_RESULT(no)
        errors=`$PKG_CONFIG --errors-to-stdout --print-errors --exists $xml_version_check`
        echo $errors
    fi
fi
])dnl

dnl
dnl APR_UTIL_VERSION_TEST
dnl
dnl $1 = prefix location of aprutil install
dnl
dnl Checks to see if the given version of aprutil
dnl is available and sets flags to indicate
dnl internal apr-util api availability
dnl
AC_DEFUN([APR_UTIL_VERSION_TEST], [

aprutil_dbd_unified_prepare_version="1.3.0"
aprutil_version_valid="no"
aprutil_version_check="apr-util-1 >= $aprutil_dbd_unified_prepare_version"

dnl Sets up the pkg-config path in case the install location
dnl is non-standard
export PKG_CONFIG_PATH=$1/lib/pkgconfig:$PKG_CONFIG_PATH


dnl Check to make sure pkg-config is available
if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
fi

if test "$PKG_CONFIG" = "no" ; then
    echo "pkg-config script not found"
else
    AC_MSG_CHECKING(for $aprutil_version_check)
    if $PKG_CONFIG --exists "$aprutil_version_check" ; then
        AC_MSG_RESULT(yes)
        APR_DBD_UNIFIED_PREPARE_FLAG="-DAPR_DBD_UNIFIED_PREPARE"
        AC_SUBST(APR_DBD_UNIFIED_PREPARE_FLAG)
    else
        AC_MSG_RESULT(no)
        errors=`$PKG_CONFIG --errors-to-stdout --print-errors --exists $aprutil_version_check`
        echo $errors
    fi
fi
])dnl

dnl
dnl JXTA_CONFIG_NICE(filename)
dnl
dnl Copied from apr_common.m4 (http://apr.apache.org)
dnl 
dnl Saves a snapshot of the configure command-line for later reuse
dnl
AC_DEFUN([JXTA_CONFIG_NICE], [
  rm -f $1
  cat >$1<<EOF
#! /bin/sh
#
# Created by configure

EOF
  if test -n "$CC"; then
    echo "CC=\"$CC\"; export CC" >> $1
  fi
  if test -n "$CFLAGS"; then
    echo "CFLAGS=\"$CFLAGS\"; export CFLAGS" >> $1
  fi
  if test -n "$CPPFLAGS"; then
    echo "CPPFLAGS=\"$CPPFLAGS\"; export CPPFLAGS" >> $1
  fi
  if test -n "$LDFLAGS"; then
    echo "LDFLAGS=\"$LDFLAGS\"; export LDFLAGS" >> $1
  fi
  if test -n "$LTFLAGS"; then
    echo "LTFLAGS=\"$LTFLAGS\"; export LTFLAGS" >> $1
  fi
  if test -n "$LIBS"; then
    echo "LIBS=\"$LIBS\"; export LIBS" >> $1
  fi
  if test -n "$INCLUDES"; then
    echo "INCLUDES=\"$INCLUDES\"; export INCLUDES" >> $1
  fi
  if test -n "$NOTEST_CFLAGS"; then
    echo "NOTEST_CFLAGS=\"$NOTEST_CFLAGS\"; export NOTEST_CFLAGS" >> $1
  fi
  if test -n "$NOTEST_CPPFLAGS"; then
    echo "NOTEST_CPPFLAGS=\"$NOTEST_CPPFLAGS\"; export NOTEST_CPPFLAGS" >> $1
  fi
  if test -n "$NOTEST_LDFLAGS"; then
    echo "NOTEST_LDFLAGS=\"$NOTEST_LDFLAGS\"; export NOTEST_LDFLAGS" >> $1
  fi
  if test -n "$NOTEST_LIBS"; then
    echo "NOTEST_LIBS=\"$NOTEST_LIBS\"; export NOTEST_LIBS" >> $1
  fi

  # Retrieve command-line arguments.
  eval "set x $[0] $ac_configure_args"
  shift

  for arg
  do
    JXTA_EXPAND_VAR(arg, $arg)
    echo "\"[$]arg\" \\" >> $1
  done
  echo '"[$]@"' >> $1
  chmod +x $1
])dnl

dnl Iteratively interpolate the contents of the second argument
dnl until interpolation offers no new result. Then assign the
dnl final result to $1.
dnl
dnl Example:
dnl
dnl foo=1
dnl bar='${foo}/2'
dnl baz='${bar}/3'
dnl JXTA_EXPAND_VAR(fraz, $baz)
dnl   $fraz is now "1/2/3"
dnl 
AC_DEFUN([JXTA_EXPAND_VAR], [
ap_last=
ap_cur="$2"
while test "x${ap_cur}" != "x${ap_last}";
do
  ap_last="${ap_cur}"
  ap_cur=`eval "echo ${ap_cur}"`
done
$1="${ap_cur}"
])


dnl If possible, we want to generate a jxta-config to make it easier 
dnl developers to compile and link programs to libjxta

dnl http://www.gnu.org/software/ac-archive/Miscellaneous/ac_create_generic_config.html
AC_DEFUN([AC_CREATE_GENERIC_CONFIG],[# create a generic PACKAGE-config file
L=`echo ifelse($1, , $PACKAGE $LIBS, $1)`
P=`echo $L | sed -e 's/ -.*//'`
P=`echo $P`
V=`echo ifelse($1, , $VERSION, $1)`
F=`echo $P-config`
AC_MSG_RESULT(creating $F - generic $V of $L)
test "x$prefix" = xNONE && prefix="$ac_default_prefix"
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
echo '#! /bin/sh' >$F
echo ' ' >>$F
echo 'package="'$P'"' >>$F
echo 'version="'$V'"' >>$F
echo 'libs="'$L'"' >>$F
echo ' ' >>$F
# in the order of occurence a standard automake Makefile
echo 'prefix="'$prefix'"' >>$F
echo 'exec_prefix="'$exec_prefix'"' >>$F
echo 'bindir="'$bindir'"' >>$F
echo 'sbindir="'$sbindir'"' >>$F
echo 'libexecdir="'$libexecdir'"' >>$F
echo 'datadir="'$datadir'"' >>$F
echo 'sysconfdir="'$sysconfdir'"' >>$F
echo 'sharedstatedir="'$sharedstatedir'"' >>$F
echo 'localstatedir="'$localstatedir'"' >>$F
echo 'libdir="'$libdir'"' >>$F
echo 'infodir="'$infodir'"' >>$F
echo 'mandir="'$mandir'"' >>$F
echo 'includedir="'$includedir'"' >>$F
echo 'target="'$target'"' >>$F
echo 'host="'$host'"' >>$F
echo 'build="'$build'"' >>$F
echo ' ' >>$F
echo 'if test "'"\$""#"'" -eq 0; then' >>$F
echo '   cat <<EOF' >>$F
echo 'Usage: $package-config [OPTIONS]' >>$F
echo 'Options:' >>$F
echo '  --prefix[=DIR]) : \$prefix' >>$F
echo '  --package) : \$package' >>$F
echo '  --version) : \$version' >>$F
echo '  --cflags) : -I\$includedir' >>$F
echo '  --libs) : -L\$libdir -l\$package' >>$F
echo '  --help) print all the options (not just these)' >>$F
echo 'EOF' >>$F
echo 'fi' >>$F
echo ' ' >>$F
echo 'o=""' >>$F
echo 'h=""' >>$F
echo 'for i ; do' >>$F
echo '  case $i in' >>$F
echo '  --prefix=*) prefix=`echo $i | sed -e "s/--prefix=//"` ;;' >>$F
echo '  --prefix)    o="$o $prefix" ;;' >>$F
echo '  --package)   o="$o $package" ;;' >>$F
echo '  --version)   o="$o $version" ;;' >>$F
echo '  --cflags) if test "_$includedir" != "_/usr/include"' >>$F
echo '          then o="$o -I$includedir" ; fi' >>$F
echo '  ;;' >>$F
echo '  --libs)      o="$o -L$libdir -l$libs" ;;' >>$F
echo '  --exec_prefix|--eprefix) o="$o $exec_prefix" ;;' >>$F
echo '  --bindir)                o="$o $bindir" ;;' >>$F
echo '  --sbindir)               o="$o $sbindir" ;;' >>$F
echo '  --libexecdir)            o="$o $libexecdir" ;;' >>$F
echo '  --datadir)               o="$o $datadir" ;;' >>$F
echo '  --datainc)               o="$o -I$datadir" ;;' >>$F
echo '  --datalib)               o="$o -L$datadir" ;;' >>$F
echo '  --sysconfdir)            o="$o $sysconfdir" ;;' >>$F
echo '  --sharedstatedir)        o="$o $sharedstatedir" ;;' >>$F
echo '  --localstatedir)         o="$o $localstatedir" ;;' >>$F
echo '  --libdir)                o="$o $libdir" ;;' >>$F
echo '  --libadd)                o="$o -L$libdir" ;;' >>$F
echo '  --infodir)               o="$o $infodir" ;;' >>$F
echo '  --mandir)                o="$o $mandir" ;;' >>$F
echo '  --target)                o="$o $target" ;;' >>$F
echo '  --host)                  o="$o $host" ;;' >>$F
echo '  --build)                 o="$o $build" ;;' >>$F
echo '  --data)                  o="$o -I$datadir/$package" ;;' >>$F
echo '  --pkgdatadir)            o="$o $datadir/$package" ;;' >>$F
echo '  --pkgdatainc)            o="$o -I$datadir/$package" ;;' >>$F
echo '  --pkgdatalib)            o="$o -L$datadir/$package" ;;' >>$F
echo '  --pkglibdir)             o="$o $libdir/$package" ;;' >>$F
echo '  --pkglibinc)             o="$o -I$libinc/$package" ;;' >>$F
echo '  --pkglibadd)             o="$o -L$libadd/$package" ;;' >>$F
echo '  --pkgincludedir)         o="$o $includedir/$package" ;;' >>$F
echo '  --help) h="1" ;;' >>$F
echo '  -?//*|-?/*//*|-?./*//*|//*|/*//*|./*//*) ' >>$F
echo '       v=`echo $i | sed -e s://:\$:g`' >>$F
echo '       v=`eval "echo $v"` ' >>$F
echo '       o="$o $v" ;; ' >>$F
echo '  esac' >>$F
echo 'done' >>$F
echo ' ' >>$F
echo 'o=`eval "echo $o"`' >>$F
echo 'o=`eval "echo $o"`' >>$F
echo 'eval "echo $o"' >>$F
echo ' ' >>$F
echo 'if test ! -z "$h" ; then ' >>$F
echo 'cat <<EOF' >>$F
echo '  --prefix=xxx)      (what is that for anyway?)' >>$F
echo '  --prefix)         \$prefix        $prefix' >>$F
echo '  --package)        \$package       $package' >>$F
echo '  --version)        \$version       $version' >>$F
echo '  --cflags)         -I\$includedir    unless it is /usr/include' >>$F
echo '  --libs)           -L\$libdir -l\$PACKAGE \$LIBS' >>$F
echo '  --exec_prefix) or... ' >>$F
echo '  --eprefix)        \$exec_prefix   $exec_prefix' >>$F
echo '  --bindir)         \$bindir        $bindir' >>$F
echo '  --sbindir)        \$sbindir       $sbindir' >>$F
echo '  --libexecdir)     \$libexecdir    $libexecdir' >>$F
echo '  --datadir)        \$datadir       $datadir' >>$F
echo '  --sysconfdir)     \$sysconfdir    $sysconfdir' >>$F
echo '  --sharedstatedir) \$sharedstatedir$sharedstatedir' >>$F
echo '  --localstatedir)  \$localstatedir $localstatedir' >>$F
echo '  --libdir)         \$libdir        $libdir' >>$F
echo '  --infodir)        \$infodir       $infodir' >>$F
echo '  --mandir)         \$mandir        $mandir' >>$F
echo '  --target)         \$target        $target' >>$F
echo '  --host)           \$host          $host' >>$F
echo '  --build)          \$build         $build' >>$F
echo '  --data)           -I\$datadir/\$package' >>$F
echo '  --pkgdatadir)     \$datadir/\$package' >>$F
echo '  --pkglibdir)      \$libdir/\$package' >>$F
echo '  --pkgincludedir)  \$includedir/\$package' >>$F
echo '  --help)           generated by ac_create_generic_config.m4' >>$F
echo '  -I//varname and other inc-targets like --pkgdatainc supported' >>$F
echo '  -L//varname and other lib-targets, e.g. --pkgdatalib or --libadd' >>$F
echo 'EOF' >>$F
echo 'fi' >>$F 
GENERIC_CONFIG="$F"
AC_SUBST(GENERIC_CONFIG)
])




dnl http://www.gnu.org/software/ac-archive/Miscellaneous/ac_path_generic.html
AC_DEFUN([AC_PATH_GENERIC],
[dnl
dnl we're going to need uppercase, lowercase and user-friendly versions of the
dnl string `LIBRARY'
pushdef([UP], translit([$1], [a-z], [A-Z]))dnl
pushdef([DOWN], translit([$1], [A-Z], [a-z]))dnl

dnl
dnl Get the cflags and libraries from the LIBRARY-config script
dnl
AC_ARG_WITH(DOWN-prefix,[  --with-]DOWN[-prefix=PFX       Prefix where $1 is installed (optional)],
        DOWN[]_config_prefix="$withval", DOWN[]_config_prefix="")
AC_ARG_WITH(DOWN-exec-prefix,[  --with-]DOWN[-exec-prefix=PFX Exec prefix where $1 is installed (optional)],
        DOWN[]_config_exec_prefix="$withval", DOWN[]_config_exec_prefix="")

  if test x$DOWN[]_config_exec_prefix != x ; then
     DOWN[]_config_args="$DOWN[]_config_args --exec-prefix=$DOWN[]_config_exec_prefix"
     if test x${UP[]_CONFIG+set} != xset ; then
       UP[]_CONFIG=$DOWN[]_config_exec_prefix/bin/DOWN-config
     fi
  fi
  if test x$DOWN[]_config_prefix != x ; then
     DOWN[]_config_args="$DOWN[]_config_args --prefix=$DOWN[]_config_prefix"
     if test x${UP[]_CONFIG+set} != xset ; then
       UP[]_CONFIG=$DOWN[]_config_prefix/bin/DOWN-config
     fi
  fi

  AC_PATH_PROG(UP[]_CONFIG, DOWN-config, no)
  ifelse([$2], ,
     AC_MSG_CHECKING(for $1),
     AC_MSG_CHECKING(for $1 - version >= $2)
  )
  no_[]DOWN=""
  if test "$UP[]_CONFIG" = "no" ; then
     no_[]DOWN=yes
  else
     UP[]_CFLAGS="`$UP[]_CONFIG $DOWN[]_config_args --cflags`"
     UP[]_LIBS="`$UP[]_CONFIG $DOWN[]_config_args --libs`"
     ifelse([$2], , ,[
        DOWN[]_config_major_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
        DOWN[]_config_minor_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
        DOWN[]_config_micro_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
        DOWN[]_wanted_major_version="regexp($2, [\<\([0-9]*\)], [\1])"
        DOWN[]_wanted_minor_version="regexp($2, [\<\([0-9]*\)\.\([0-9]*\)], [\2])"
        DOWN[]_wanted_micro_version="regexp($2, [\<\([0-9]*\).\([0-9]*\).\([0-9]*\)], [\3])"

        # Compare wanted version to what config script returned.
        # If I knew what library was being run, i'd probably also compile
        # a test program at this point (which also extracted and tested
        # the version in some library-specific way)
        if test "$DOWN[]_config_major_version" -lt \
                        "$DOWN[]_wanted_major_version" \
          -o \( "$DOWN[]_config_major_version" -eq \
                        "$DOWN[]_wanted_major_version" \
            -a "$DOWN[]_config_minor_version" -lt \
                        "$DOWN[]_wanted_minor_version" \) \
          -o \( "$DOWN[]_config_major_version" -eq \
                        "$DOWN[]_wanted_major_version" \
            -a "$DOWN[]_config_minor_version" -eq \
                        "$DOWN[]_wanted_minor_version" \
            -a "$DOWN[]_config_micro_version" -lt \
                        "$DOWN[]_wanted_micro_version" \) ; then
          # older version found
          no_[]DOWN=yes
          echo -n "*** An old version of $1 "
          echo -n "($DOWN[]_config_major_version"
          echo -n ".$DOWN[]_config_minor_version"
          echo    ".$DOWN[]_config_micro_version) was found."
          echo -n "*** You need a version of $1 newer than "
          echo -n "$DOWN[]_wanted_major_version"
          echo -n ".$DOWN[]_wanted_minor_version"
          echo    ".$DOWN[]_wanted_micro_version."
          echo "***"
          echo "*** If you have already installed a sufficiently new version, this error"
          echo "*** probably means that the wrong copy of the DOWN-config shell script is"
          echo "*** being found. The easiest way to fix this is to remove the old version"
          echo "*** of $1, but you can also set the UP[]_CONFIG environment to point to the"
          echo "*** correct copy of DOWN-config. (In this case, you will have to"
          echo "*** modify your LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf"
          echo "*** so that the correct libraries are found at run-time)"
        fi
     ])
  fi
  if test "x$no_[]DOWN" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$3], , :, [$3])
  else
     AC_MSG_RESULT(no)
     if test "$UP[]_CONFIG" = "no" ; then
       echo "*** The DOWN-config script installed by $1 could not be found"
       echo "*** If $1 was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the UP[]_CONFIG environment variable to the"
       echo "*** full path to DOWN-config."
     fi
     UP[]_CFLAGS=""
     UP[]_LIBS=""
     ifelse([$4], , :, [$4])
  fi
  AC_SUBST(UP[]_CFLAGS)
  AC_SUBST(UP[]_LIBS)

  popdef([UP])
  popdef([DOWN])
])
