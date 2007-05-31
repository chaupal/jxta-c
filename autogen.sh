#!/bin/sh

RECOMMENDED_AUTOCONF_VER=2.59
REQUIRED_AUTOCONF_VER=2.53
RECOMMENDED_AUTOMAKE_VER=1.7
REQUIRED_AUTOMAKE_VER=1.7
RECOMMENDED_LIBTOOL_VER=1.4.3
REQUIRED_LIBTOOL_VER=1.3.5

AM_PROGS="automake-1.7 automake-1.8 automake-1.9 automake"
AC_PROGS="autoconf2.50 autoconf"

# compare 2 versions 
# return true if $1 is later than $2
ver_compare() {
    cur_ver=$1
    min_ver=$2
    result=0
    saved_IFS="$IFS"; IFS="."
    set $min_ver
    for x in $cur_ver; do
        y=$1; shift;
        if [ -z "$x" ]; then result=1; break; fi
        if [ -z "$y" ]; then break; fi
        if [ $x -gt $y ]; then break; fi
        if [ $x -lt $y ]; then result=1; break; fi
    done
    IFS="$saved_IFS"
    return $result
}

# Check program is later then version
# Usage: ver_check PROG RECOMMENDED_VER MINIMUM_VER
ver_check() {
    vc_prog=$1
    vc_r_ver=$2
    vc_min_ver=$3
    vc_result=1

    echo "checking if $vc_prog >= $vc_r_ver ..."
    vc_prog_ver=`$vc_prog --version | head -n 1 | \
                           sed 's/^.*[ \t]\([0-9.]*\).*$/\1/'`
    if ver_compare $vc_prog_ver $vc_r_ver; then
        echo "found $vc_prog_ver"
        vc_result=0
    elif [ -n "$vc_min_ver" ] && ver_compare $vc_prog_ver $vc_min_ver; then
        echo "found $vc_prog_ver is OK, but $vc_r_ver is recommended"
        vc_result=0
    else
        echo "ERROR: too old (found version $vc_prog_version)"
    fi
    return $vc_result
}

# Pick the program in preference, the first meet minimum version 
# requirement will be picked.
# Usage: prog_pick PROG_VAR PROG_LIST RECOMMEND_VER MINIMUM_VER
prog_pick() {
    prog_var=$1
    vc_result=1

    for prog in $2; do
        if $prog --version < /dev/null > /dev/null 2>&1; then
            if ver_check $prog $3 $4; then
                eval "$prog_var=$prog"
                vc_result=0
                break;
            fi
        fi
    done
    if [ "$vc_result" != 0 ]; then
        echo ""
        echo "*ERROR*: You must have $prog >= $3 installed"
        echo "  to build jxta-c."
        echo ""
    fi
    return $vc_result
}

DIE=0

ver_check libtoolize $RECOMMENDED_LIBTOOL_VER $REQUIRED_LIBTOOL_VER || DIE=1
prog_pick AUTOMAKE "$AM_PROGS" $RECOMMENDED_AUTOMAKE_VER $REQUIRED_AUTOMAKE_VER || DIE=1
prog_pick AUTOCONF "$AC_PROGS" $RECOMMENDED_AUTOCONF_VER $REQUIRED_AUTOCONF_VER || DIE=1
ACLOCAL=`echo $AUTOMAKE | sed s/automake/aclocal/`
AUTOHEADER=`echo $AUTOCONF | sed s/autoconf/autoheader/`

#ver_check autoconf $RECOMMENDED_AUTOCONF_VER $REQUIRED_AUTOCONF_VER || DIE=1
#ver_check automake $RECOMMENDED_AUTOMAKE_VER $REQUIRED_AUTOMAKE_VER || DIE=1

if [ "$DIE" -eq 1 ]; then
    exit 1
fi

if [ ! -d conftools ]; then
    mkdir conftools
fi

echo "Running libtoolize ..." 
libtoolize --force --automake
echo "Running $ACLOCAL ..." 
$ACLOCAL $ACLOCAL_FLAGS || exit;
echo "Running $AUTOHEADER ..." 
$AUTOHEADER || exit;
echo "Running $AUTOMAKE first round ..." 
$AUTOMAKE --add-missing --copy
echo "Running $AUTOCONF ..." 
$AUTOCONF || exit;
echo "Running $AUTOMAKE second round ..." 
$AUTOMAKE
echo "Running configure $@ ..."
./configure $@

