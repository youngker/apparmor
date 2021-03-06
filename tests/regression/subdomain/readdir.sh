#! /bin/bash
# $Id$

#	Copyright (C) 2002-2005 Novell/SUSE
#
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2 of the
#	License.

#=NAME readdir
#=DESCRIPTION 
# AppArmor requires 'r' permission on a directory in order for a confined task 
# to be able to read the directory contents.  This test verifies this.
#=END

pwd=`dirname $0`
pwd=`cd $pwd ; /bin/pwd`

bin=$pwd

. $bin/prologue.inc

dir=$tmpdir/tmpdir
# x is not really needed, see chdir.sh
okperm=rix
badperm=ix

mkdir $dir

# CHDIR TEST

genprofile $dir/:$okperm

runchecktest "READDIR" pass $dir

# CHDIR TEST (no perm)

genprofile $dir/:$badperm

runchecktest "READDIR (no perm)" fail $dir
