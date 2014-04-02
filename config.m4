PHP_ARG_ENABLE(epv, whether to enable epv support,
[  --enable-epv           Enable epv support])

if test "$PHP_MU" != "no"; then
  PHP_NEW_EXTENSION(epv, epv.c, $ext_shared)
  CXXFLAGS="-O3 -fno-strict-aliasing"
  CFLAGS="-O3 -fno-strict-aliasing"
fi
