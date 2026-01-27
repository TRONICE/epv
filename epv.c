#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/url.h"
#include "php_epv.h"
#include "main/php_variables.h"
#include "main/php_globals.h"
#include "main/SAPI.h"
#include "main/php_streams.h"
#include "main/rfc1867.h"
#include "main/php_content_types.h"

ZEND_DECLARE_MODULE_GLOBALS(epv)

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("epv.max_input_size", "0", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateLong, max_input_size, zend_epv_globals, epv_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_epv_init_globals
 */
static void php_epv_init_globals(void *epv_globals_ptr)
{
	zend_epv_globals *epv_globals = (zend_epv_globals *)epv_globals_ptr;
	memset(epv_globals, 0, sizeof(zend_epv_globals));
	epv_globals->max_input_size = 0;
}
/* }}} */

/* {{{ epv_read_request_data
 * Read request body data for PUT/DELETE/PATCH methods
 * Similar to sapi_read_standard_form_data() for POST
 */
static void epv_read_request_data(void)
{
	zend_long max_size = EPV_G(max_input_size);
	zend_long content_length = SG(request_info).content_length;

	/* Check if request_body is already buffered */
	if (SG(request_info).request_body) {
		return;
	}

	/* Check if we have a read_post method */
	if (!sapi_module.read_post) {
		return;
	}

	/* Check max_input_size limit if set (0 = unlimited) */
	if (max_size > 0 && content_length > max_size) {
		php_error_docref(NULL, E_WARNING,
			"Request Content-Length of " ZEND_LONG_FMT " bytes exceeds the limit of " ZEND_LONG_FMT " bytes",
			content_length, max_size);
		return;
	}

	/* Create a temporary stream to buffer the request body */
	SG(request_info).request_body = php_stream_temp_create_ex(
		TEMP_STREAM_DEFAULT,
		SAPI_POST_BLOCK_SIZE,
		PG(upload_tmp_dir)
	);

	if (!SG(request_info).request_body) {
		php_error_docref(NULL, E_WARNING, "Failed to create request body stream");
		return;
	}

	/* Read data from SAPI in blocks */
	for (;;) {
		char buffer[SAPI_POST_BLOCK_SIZE];
		size_t read_bytes;

		/* Read one block of data */
		read_bytes = sapi_read_post_block(buffer, SAPI_POST_BLOCK_SIZE);

		if (read_bytes > 0) {
			/* Write to the stream */
			if (php_stream_write(SG(request_info).request_body, buffer, read_bytes) != read_bytes) {
				/* Write failed, purge the stream */
				php_stream_truncate_set_size(SG(request_info).request_body, 0);
				php_error_docref(NULL, E_WARNING, "Request data can't be buffered; all data discarded");
				break;
			}
		}

		/* Check if we've exceeded the limit after reading */
		if (max_size > 0 && SG(read_post_bytes) > max_size) {
			php_error_docref(NULL, E_WARNING,
				"Actual request length does not match Content-Length, and exceeds " ZEND_LONG_FMT " bytes",
				max_size);
			break;
		}

		/* Check if we're done reading */
		if (read_bytes < SAPI_POST_BLOCK_SIZE) {
			break;
		}
	}

	/* Rewind the stream for reading */
	php_stream_rewind(SG(request_info).request_body);
}
/* }}} */

/* {{{ parse_http_method_data
 * Parse request body data similar to POST handling
 * Supports multiple Content-Types: urlencoded and multipart
 */
static void parse_http_method_data(zval *arr)
{
	const char *content_type = SG(request_info).content_type;
	sapi_post_entry *post_entry;
	size_t content_type_length;
	char *content_type_normalized;
	char *p;
	char oldchar = 0;
	char *saved_content_type_dup = NULL;
	sapi_post_entry *saved_post_entry = NULL;
	zval saved_post_array;
	zend_bool swapped_post_array = 0;

	/* Check if we have a Content-Type */
	if (!content_type || !*content_type) {
		/* No Content-Type, cannot determine how to parse data */
		return;
	}

	/* Normalize content type - make lowercase and trim parameters */
	content_type_length = strlen(content_type);
	content_type_normalized = estrndup(content_type, content_type_length);

	for (p = content_type_normalized; p < content_type_normalized + content_type_length; p++) {
		switch (*p) {
			case ';':
			case ',':
			case ' ':
				content_type_length = p - content_type_normalized;
				oldchar = *p;
				*p = 0;
				break;
			default:
				*p = tolower(*p);
				break;
		}
	}

	/* Look up the POST content handler */
	post_entry = zend_hash_str_find_ptr(&SG(known_post_content_types),
		content_type_normalized, content_type_length);

	if (!post_entry) {
		/* No handler found for this content type */
		efree(content_type_normalized);
		return;
	}

	/* Restore the original character if we modified it */
	if (oldchar) {
		*(p - 1) = oldchar;
	}

	/* Save current SAPI state (in case it was set by actual POST handling) */
	saved_post_entry = SG(request_info).post_entry;
	saved_content_type_dup = SG(request_info).content_type_dup;

	/* Safety check: content_type_dup should be NULL at RINIT stage for PUT/DELETE/PATCH */
	if (saved_content_type_dup != NULL) {
		/* This is unexpected - another component has already set this */
		php_error_docref(NULL, E_WARNING,
			"EPV: content_type_dup already set before processing %s request, possible conflict",
			SG(request_info).request_method);
		efree(content_type_normalized);
		return;
	}

	/* Set up SAPI state for our handler */
	SG(request_info).post_entry = post_entry;
	SG(request_info).content_type_dup = estrdup(content_type);

	/* For urlencoded content, we need to read the request body first */
	if (post_entry->post_reader) {
		if (!SG(request_info).request_body) {
			epv_read_request_data();
		}
	}

	/* IMPORTANT: rfc1867_post_handler writes directly to PG(http_globals)[TRACK_VARS_POST] */
	/* So we need to temporarily swap it with our target array */
	/* This is SAFE because:
	 * 1. We only do this for PUT/DELETE/PATCH requests (not POST)
	 * 2. At RINIT stage, POST array is still UNDEF
	 * 3. We restore it immediately after processing
	 * 4. When $_POST is later accessed, auto-global callback will initialize it properly
	 */

	/* Save the current POST array state */
	ZVAL_COPY_VALUE(&saved_post_array, &PG(http_globals)[TRACK_VARS_POST]);

	/* Verify we're not breaking an already-initialized POST array */
	if (Z_TYPE(saved_post_array) != IS_UNDEF && Z_TYPE(saved_post_array) != IS_ARRAY) {
		php_error_docref(NULL, E_WARNING,
			"EPV: Unexpected POST array type during processing, data may be incomplete");
		efree(content_type_normalized);
		return;
	}

	/* Swap our target array into the POST position */
	ZVAL_COPY_VALUE(&PG(http_globals)[TRACK_VARS_POST], arr);
	swapped_post_array = 1;

	/* Call the post handler through sapi_handle_post */
	/* Note: sapi_handle_post will efree content_type_dup and set it to NULL */
	sapi_handle_post(arr);

	/* Restore the original POST array */
	if (swapped_post_array) {
		/* Copy data from our target array (which is now in PG POST position) back to arr */
		ZVAL_COPY_VALUE(arr, &PG(http_globals)[TRACK_VARS_POST]);
		/* Restore the original POST array */
		ZVAL_COPY_VALUE(&PG(http_globals)[TRACK_VARS_POST], &saved_post_array);
	}

	/* Restore SAPI state */
	SG(request_info).post_entry = saved_post_entry;
	/* content_type_dup was already freed by sapi_handle_post */
	SG(request_info).content_type_dup = saved_content_type_dup;

	efree(content_type_normalized);
}
/* }}} */

/* {{{ php_auto_globals_create_put
 * Auto-global callback for $_PUT
 */
static zend_bool php_auto_globals_create_put(zend_string *name)
{
	zval *put_array_ptr = &EPV_G(http_globals)[TRACK_VARS_PUT];

	/* If the array hasn't been initialized yet (e.g., not a PUT request), */
	/* initialize it as an empty array */
	if (Z_TYPE_P(put_array_ptr) == IS_UNDEF) {
		array_init(put_array_ptr);
	}

	zend_hash_update(&EG(symbol_table), name, put_array_ptr);
	Z_ADDREF_P(put_array_ptr);

	return 0; /* don't rearm */
}
/* }}} */

/* {{{ php_auto_globals_create_delete
 * Auto-global callback for $_DELETE
 */
static zend_bool php_auto_globals_create_delete(zend_string *name)
{
	zval *delete_array_ptr = &EPV_G(http_globals)[TRACK_VARS_DELETE];

	/* If the array hasn't been initialized yet (e.g., not a DELETE request), */
	/* initialize it as an empty array */
	if (Z_TYPE_P(delete_array_ptr) == IS_UNDEF) {
		array_init(delete_array_ptr);
	}

	zend_hash_update(&EG(symbol_table), name, delete_array_ptr);
	Z_ADDREF_P(delete_array_ptr);

	return 0; /* don't rearm */
}
/* }}} */

/* {{{ php_auto_globals_create_patch
 * Auto-global callback for $_PATCH
 */
static zend_bool php_auto_globals_create_patch(zend_string *name)
{
	zval *patch_array_ptr = &EPV_G(http_globals)[TRACK_VARS_PATCH];

	/* If the array hasn't been initialized yet (e.g., not a PATCH request), */
	/* initialize it as an empty array */
	if (Z_TYPE_P(patch_array_ptr) == IS_UNDEF) {
		array_init(patch_array_ptr);
	}

	zend_hash_update(&EG(symbol_table), name, patch_array_ptr);
	Z_ADDREF_P(patch_array_ptr);

	return 0; /* don't rearm */
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(epv)
{
	REGISTER_INI_ENTRIES();

	/* Register auto-globals */
	zend_register_auto_global(
		zend_string_init_interned("_PUT", sizeof("_PUT") - 1, 1),
		0,
		php_auto_globals_create_put
	);

	zend_register_auto_global(
		zend_string_init_interned("_DELETE", sizeof("_DELETE") - 1, 1),
		0,
		php_auto_globals_create_delete
	);

	zend_register_auto_global(
		zend_string_init_interned("_PATCH", sizeof("_PATCH") - 1, 1),
		0,
		php_auto_globals_create_patch
	);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(epv)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(epv)
{
	const char *request_method;

#if defined(COMPILE_DL_EPV) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	/* DON'T initialize arrays here - they might have been set by early auto-global access */
	/* We'll check and initialize them on-demand when processing the request method */

	/* Check request method and pre-populate the appropriate array */
	/* This must be done early, before any code accesses $_POST, */
	/* because multipart request bodies can only be read once */
	request_method = SG(request_info).request_method;

	if (request_method) {
		if (!strcasecmp(request_method, "PUT")) {
			/* Check if array was already initialized by early auto-global access */
			if (Z_TYPE(EPV_G(http_globals)[TRACK_VARS_PUT]) == IS_UNDEF) {
				array_init(&EPV_G(http_globals)[TRACK_VARS_PUT]);
			}
			parse_http_method_data(&EPV_G(http_globals)[TRACK_VARS_PUT]);
		} else if (!strcasecmp(request_method, "DELETE")) {
			if (Z_TYPE(EPV_G(http_globals)[TRACK_VARS_DELETE]) == IS_UNDEF) {
				array_init(&EPV_G(http_globals)[TRACK_VARS_DELETE]);
			}
			parse_http_method_data(&EPV_G(http_globals)[TRACK_VARS_DELETE]);
		} else if (!strcasecmp(request_method, "PATCH")) {
			if (Z_TYPE(EPV_G(http_globals)[TRACK_VARS_PATCH]) == IS_UNDEF) {
				array_init(&EPV_G(http_globals)[TRACK_VARS_PATCH]);
			}
			parse_http_method_data(&EPV_G(http_globals)[TRACK_VARS_PATCH]);
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(epv)
{
	/* Clean up the arrays */
	if (Z_TYPE(EPV_G(http_globals)[TRACK_VARS_PUT]) != IS_UNDEF) {
		zval_ptr_dtor(&EPV_G(http_globals)[TRACK_VARS_PUT]);
		ZVAL_UNDEF(&EPV_G(http_globals)[TRACK_VARS_PUT]);
	}
	if (Z_TYPE(EPV_G(http_globals)[TRACK_VARS_DELETE]) != IS_UNDEF) {
		zval_ptr_dtor(&EPV_G(http_globals)[TRACK_VARS_DELETE]);
		ZVAL_UNDEF(&EPV_G(http_globals)[TRACK_VARS_DELETE]);
	}
	if (Z_TYPE(EPV_G(http_globals)[TRACK_VARS_PATCH]) != IS_UNDEF) {
		zval_ptr_dtor(&EPV_G(http_globals)[TRACK_VARS_PATCH]);
		ZVAL_UNDEF(&EPV_G(http_globals)[TRACK_VARS_PATCH]);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(epv)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "EPV Support", "enabled");
	php_info_print_table_row(2, "Version", PHP_EPV_VERSION);
	php_info_print_table_row(2, "Supported Methods", "PUT, DELETE, PATCH");
	php_info_print_table_row(2, "Superglobal Variables", "$_PUT, $_DELETE, $_PATCH");
	php_info_print_table_row(2, "Supported Content-Types", "application/x-www-form-urlencoded, multipart/form-data");
	php_info_print_table_row(2, "File Uploads", "Supported (via $_FILES)");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ epv_module_entry
 */
zend_module_entry epv_module_entry = {
	STANDARD_MODULE_HEADER,
	"epv",
	NULL, /* functions */
	PHP_MINIT(epv),
	PHP_MSHUTDOWN(epv),
	PHP_RINIT(epv),
	PHP_RSHUTDOWN(epv),
	PHP_MINFO(epv),
	PHP_EPV_VERSION,
	PHP_MODULE_GLOBALS(epv),
	php_epv_init_globals,
	NULL, /* globals dtor */
	NULL, /* post deactivate */
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_EPV
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(epv)
#endif
