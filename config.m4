dnl $Id$
dnl config.m4 for extension epv

PHP_ARG_ENABLE(epv, whether to enable epv support,
[  --enable-epv           Enable EPV support (PUT, DELETE, PATCH)])

if test "$PHP_EPV" != "no"; then
  PHP_NEW_EXTENSION(epv, epv.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
