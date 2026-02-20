#ifndef PHP_EPV_H
#define PHP_EPV_H

extern zend_module_entry epv_module_entry;
#define phpext_epv_ptr &epv_module_entry

#define PHP_EPV_VERSION "3.0.0"

#ifdef PHP_WIN32
#	define PHP_EPV_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_EPV_API __attribute__ ((visibility("default")))
#else
#	define PHP_EPV_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/* Track vars indices for our new superglobals */
#define TRACK_VARS_PUT    0
#define TRACK_VARS_DELETE 1
#define TRACK_VARS_PATCH  2

ZEND_BEGIN_MODULE_GLOBALS(epv)
	zval http_globals[3]; /* PUT, DELETE, PATCH */
	zend_long max_input_size; /* Maximum input size for PUT/DELETE/PATCH (0 = unlimited) */
ZEND_END_MODULE_GLOBALS(epv)

#ifdef ZTS
#define EPV_G(v) TSRMG(epv_globals_id, zend_epv_globals *, v)
#else
#define EPV_G(v) (epv_globals.v)
#endif

#if defined(ZTS) && defined(COMPILE_DL_EPV)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_EPV_H */
