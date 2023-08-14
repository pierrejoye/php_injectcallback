dnl config.m4 for extension injectcb

PHP_ARG_ENABLE(injectcb, whether to enable injectcb support,
[  --enable-injectcb          Enable injectcb support], no)

if test "$PHP_INJECTCB" != "no"; then
  PHP_NEW_EXTENSION(injectcb, injectcb.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
