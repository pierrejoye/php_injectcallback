/* injectcb extension for PHP */

#ifndef PHP_INJECTCB_H
# define PHP_INJECTCB_H

extern zend_module_entry injectcb_module_entry;
# define phpext_injectcv_ptr &injectcb_module_entry

# define PHP_INJECTCB_VERSION "0.0.1"

# if defined(ZTS) && defined(COMPILE_DL_INJECTCB)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_INJECTCB_H */
