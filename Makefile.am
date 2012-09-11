bin_PROGRAMS=slp
slp_SOURCES=scanner.l parser.c slp.c xmalloc.c system.h aclocal.m4 slp.1
slp_CFLAGS=-g -Wall 

#  uncomment the following if slp requires the math library
#slp_LDADD=-lm

EXTRA_DIST=slp.lsm.in slp.spec.in slp.texinfo

#  if you write a self-test script named `chk', uncomment the
#  following and add `chk' to the EXTRA_DIST list
#TESTS=chk

#  build and install the .info pages
info_TEXINFOS = slp.texinfo
slp_TEXINFOS = gpl.texinfo

#  install the man pages
man_MANS=slp.1
