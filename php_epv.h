#ifndef PHP_EPV_H
#define PHP_EPV_H

extern zend_module_entry epv_module_entry;
#define phpext_epv_ptr &epv_module_entry

#define EPV_VERSION "1.0.1"

#define EPV_BUFFER_SIZE 512


PHP_MINIT_FUNCTION(epv);
PHP_MSHUTDOWN_FUNCTION(epv);
PHP_RINIT_FUNCTION(epv);
PHP_RSHUTDOWN_FUNCTION(epv);
PHP_MINFO_FUNCTION(epv);

ZEND_BEGIN_MODULE_GLOBALS(epv)
ZEND_END_MODULE_GLOBALS(epv)

extern ZEND_DECLARE_MODULE_GLOBALS(epv);

#endif
/* PHP_EPV_H */
