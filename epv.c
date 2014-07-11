#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/url.h"
#include "php_epv.h"
#include "stdio.h"
#include "php_variables.h"
#include "php_open_temporary_file.h"

ZEND_DECLARE_MODULE_GLOBALS(epv);

static void epv_register_auto_global(TSRMLS_D);
static void epv_init_auto_global(TSRMLS_D);


static void epv_register_auto_global(TSRMLS_D)
{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
		zend_register_auto_global("_PUT", sizeof("_PUT") - 1, NULL TSRMLS_CC);
		zend_register_auto_global("_DELETE", sizeof("_DELETE") - 1, NULL TSRMLS_CC);
#else
		zend_register_auto_global("_PUT", sizeof("_PUT") - 1, 0, NULL TSRMLS_CC);
		zend_register_auto_global("_DELETE", sizeof("_DELETE") - 1, 0, NULL TSRMLS_CC);
#endif
}

static void epv_init_auto_global(TSRMLS_D)
{
		zval * auto_global_server, ** request_method_pointer, ** content_type_pointer;
		zval * put, * delete, * request_method, * content_type;

		MAKE_STD_ZVAL(put);
		array_init(put);
		zend_hash_add(&EG(symbol_table), "_PUT", 5, &put, sizeof(zval *), NULL);
		Z_ADDREF_P(put);

		MAKE_STD_ZVAL(delete);
		array_init(delete);
		zend_hash_add(&EG(symbol_table), "_DELETE", 8, &delete, sizeof(zval *), NULL);
		Z_ADDREF_P(delete);


#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
		if( (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays)) )
		{
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
		}
#else
		if( PG(auto_globals_jit) )
		{
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
		}
#endif

		auto_global_server = PG(http_globals)[TRACK_VARS_SERVER];


		if(zend_hash_find(HASH_OF(auto_global_server), "REQUEST_METHOD", 15, (void **)&request_method_pointer) == FAILURE )
		{
				MAKE_STD_ZVAL(request_method);
				ZVAL_NULL(request_method);
		}
		else
		{
				Z_ADDREF_P(*request_method_pointer);
				request_method = *request_method_pointer;
		}


		if(zend_hash_find(HASH_OF(auto_global_server), "CONTENT_TYPE", 13, (void **)&content_type_pointer) == FAILURE )
		{
				MAKE_STD_ZVAL(content_type);
				ZVAL_NULL(content_type);
		}
		else
		{
				Z_ADDREF_P(*content_type_pointer);
				content_type = *content_type_pointer;
		}


		if(
				(!ZVAL_IS_NULL(request_method) && Z_TYPE_P(request_method) == IS_STRING) &&
				(strcasecmp(Z_STRVAL_P(request_method), "put") == 0 || strcasecmp(Z_STRVAL_P(request_method), "delete") == 0) &&
				(!ZVAL_IS_NULL(content_type) &&	Z_TYPE_P(content_type) == IS_STRING)
		)
		{
				//Get php://input
				zval * function, * input_content;
				zval * php_input, * parse_result;
				zval * function_params[1];	

				MAKE_STD_ZVAL(function);
				ZVAL_STRING(function, "file_get_contents", 1);

				MAKE_STD_ZVAL(input_content);

				MAKE_STD_ZVAL(php_input);
				ZVAL_STRING(php_input, "php://input", 1);

				function_params[0] = php_input;

				if(call_user_function(EG(function_table), NULL, function, input_content, 1, function_params TSRMLS_CC) == FAILURE)
				{
						zval_ptr_dtor(&function);
						zval_ptr_dtor(&input_content);
						zval_ptr_dtor(&php_input);

						php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not get php://input");
				}

				zval_ptr_dtor(&function);
				zval_ptr_dtor(&php_input);


				char * input_data = malloc( Z_STRLEN_P(input_content)  + 1);
				memcpy(input_data, Z_STRVAL_P(input_content), Z_STRLEN_P(input_content));
				input_data[Z_STRLEN_P(input_content)] = 0;

				char * input_data_end = input_data + Z_STRLEN_P(input_content);

				zval_ptr_dtor(&input_content);




				if(strncasecmp(Z_STRVAL_P(content_type), "application/x-www-form-urlencoded", 33) == 0)
				{
						//Parse data
						MAKE_STD_ZVAL(parse_result);
						array_init(parse_result);

						char *input_parameter = estrndup(input_data, strlen(input_data));

						sapi_module.treat_data(PARSE_STRING, input_parameter, parse_result TSRMLS_CC);


						if(!ZVAL_IS_NULL(parse_result) && Z_TYPE_P(parse_result) == IS_ARRAY)
						{
								if( strcasecmp(Z_STRVAL_P(request_method), "put") == 0)
								{
										zend_hash_copy(Z_ARRVAL_P(put), Z_ARRVAL_P(parse_result), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
										zend_hash_add(&EG(symbol_table), "_PUT", 5, &put, sizeof(zval *), NULL);
								}
								else if( strcasecmp(Z_STRVAL_P(request_method), "delete") == 0)
								{
										zend_hash_copy(Z_ARRVAL_P(delete), Z_ARRVAL_P(parse_result), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
										zend_hash_add(&EG(symbol_table), "_DELETE", 8, &delete, sizeof(zval *), NULL);
								}
						}
				}
				else if(strncasecmp(Z_STRVAL_P(content_type), "multipart/form-data", 19) == 0)
				{
						char *input_data_boundary_end = zend_memnstr(input_data, "\r\n", 2, input_data_end);

						char *boundary = input_data;
						*input_data_boundary_end = 0;

						int boundary_len = input_data_boundary_end - input_data;


						
						char *current_input_data = input_data_boundary_end + 2; //bypass tail "\r\n"
						
						char header_lower[EPV_BUFFER_SIZE];
						char *header, *header_end, *data, *header_value;
						char *data_name, *data_name_end;
						char *input_parameter_string = NULL, *input_parameter_string_old;
						long current_part_len, current_data_len;

						char *part_end = zend_memnstr(current_input_data, boundary, boundary_len, input_data_end);
						while(part_end != NULL)
						{
								*part_end = 0; //ignore boundary

								current_part_len = part_end - current_input_data;
								if(strcmp(current_input_data + current_part_len - 2, "\r\n") == 0)
								{
										*(current_input_data + current_part_len - 2) = 0; //ignore tail "\r\n"
										current_part_len -= 2; //ignore tail "\r\n"
								}

								//If this is the last part, break
								if(strcmp(current_input_data, "--") == 0)
								{
										break;
								}

								if(strncmp(current_input_data, "\r\n", 2) == 0)
								{
										current_input_data += 2; //bypass head "\r\n"
								}



								if((header_end = strstr(current_input_data, "\r\n\r\n")) != NULL)
								{
										*(header_end + 2) = 0; //ignore one tail "\r\n" and leave one tail "\r\n" for sub-header parse
										header = current_input_data;
										data = current_input_data + strlen(header) + 2; //bypass left tail "\r\n"
										current_data_len = part_end - data - 2; //by pass "\r\n


										memcpy(header_lower, header, header_end - header);
										header_lower[header_end - header] = 0;
										php_strtolower(header_lower, header_end - header);



										//File
										if(strstr(header_lower, "filename=\"") != NULL)
										{
												zval * files_entry, *uniqid, ** ppzval, * auto_global_files;
												FILE * file_handler;
												zend_bool ini_exists;

												char upload_max_filesize_string[EPV_BUFFER_SIZE], tmp_file[EPV_BUFFER_SIZE];
												char upload_max_filesize_unit;

												char sub_content_disposition[EPV_BUFFER_SIZE], sub_content_type[EPV_BUFFER_SIZE];
												char *sub_header, *sub_header_end;
												char *filename, *filename_end, *upload_tmp_dir = NULL, *upload_max_filesize_ini;
												long upload_max_filesize;

												sub_header = header;
												sub_header_end = strstr(header, "\r\n");
												while(sub_header_end != NULL)
												{
														memcpy(header_lower, sub_header, sub_header_end - sub_header);
														header_lower[sub_header_end - sub_header] = 0;
														php_strtolower(header_lower, sub_header_end - sub_header);

														if(strstr(header_lower, "content-disposition") != NULL)
														{
																header_value = strstr(sub_header, ": ");
																header_value += 2; //bypass ": "
																memcpy(sub_content_disposition, header_value, sub_header_end - header_value);
																sub_content_disposition[sub_header_end - header_value] = 0;
														}
														else if(strstr(header_lower, "content-type") != NULL)
														{
																header_value = strstr(sub_header, ": ");
																header_value += 2; //bypass ": "
																memcpy(sub_content_type, header_value, sub_header_end - header_value);
																sub_content_type[sub_header_end - header_value] = 0;
														}

														sub_header = sub_header_end + 2; //bypass "\r\n" shift to next sub_header
														sub_header_end = strstr(sub_header, "\r\n");
												}

												data_name = strstr(sub_content_disposition, "name=\"");

												if(data_name != NULL)
												{
														data_name += 6; //bypass "name=\""
														data_name_end = strstr(data_name, "\"");
														*data_name_end = 0; //ignore "\""


														filename = strstr(data_name_end + 1, "filename=\"");
														if(filename != NULL)
														{
																filename += 10; //bypass "filename="\"
																filename_end = strstr(filename, "\"");
																*filename_end = 0; //ignore "\""


																upload_max_filesize_ini = zend_ini_string_ex("upload_max_filesize", 20, 0, &ini_exists);

																long upload_max_filesize_ini_len = strlen(upload_max_filesize_ini);
																memcpy(upload_max_filesize_string, upload_max_filesize_ini, upload_max_filesize_ini_len);
																upload_max_filesize_string[upload_max_filesize_ini_len] = 0;

																long upload_max_filesize_string_len = strlen(upload_max_filesize_string);
																php_strtolower(upload_max_filesize_string, upload_max_filesize_string_len);

																upload_max_filesize_unit = upload_max_filesize_string[upload_max_filesize_string_len - 1]; //get last character
																upload_max_filesize_string[upload_max_filesize_string_len - 1] = 0; //ignore last character


																upload_max_filesize = atoi(upload_max_filesize_string);

																switch(upload_max_filesize_unit)
																{
																		case 'g':
																				upload_max_filesize *= 1024;
																		case 'm':
																				upload_max_filesize *= 1024;
																		case 'k':
																				upload_max_filesize *= 1024;
																}



																//Get $_FILES
																if(zend_hash_find(&EG(symbol_table), ZEND_STRS("_FILES"), (void **)&ppzval) == FAILURE)
																{
																		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not get $_FILES");
																}
																else
																{
																		auto_global_files = *ppzval;
																}


																//Make $_FILES entry
																MAKE_STD_ZVAL(files_entry);
																array_init(files_entry);
																add_assoc_string(files_entry, "name", filename, 1);


																//Check upload_max_filesize
																if(current_data_len <= upload_max_filesize)
																{
																		upload_tmp_dir = zend_ini_string_ex("upload_tmp_dir", 15, 0, &ini_exists);
																		if(upload_tmp_dir == NULL)
																		{
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 5)
																				upload_tmp_dir = (char *)php_get_temporary_directory();
#else
																				upload_tmp_dir = (char *)php_get_temporary_directory(TSRMLS_C);
#endif
																		}




																		//Get unique id
																		MAKE_STD_ZVAL(function);
																		ZVAL_STRING(function, "uniqid", 1);

																		MAKE_STD_ZVAL(uniqid);

																		if(call_user_function(EG(function_table), NULL, function, uniqid, 0, NULL TSRMLS_CC) == FAILURE)
																		{
																				zval_ptr_dtor(&function);
																				zval_ptr_dtor(&uniqid);

																				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not get uniqid");
																		}

																		if(upload_tmp_dir[strlen(upload_tmp_dir) - 1] != '/')
																		{
																				sprintf(tmp_file, "%s/%s", upload_tmp_dir, Z_STRVAL_P(uniqid));
																		}
																		else
																		{
																				sprintf(tmp_file, "%s%s", upload_tmp_dir, Z_STRVAL_P(uniqid));
																		}

																		zval_ptr_dtor(&function);
																		zval_ptr_dtor(&uniqid);




																		file_handler = fopen(tmp_file, "wb");
																		fwrite(data, sizeof(char), current_data_len, file_handler);
																		fclose(file_handler);


																		add_assoc_string(files_entry, "type", sub_content_type, 1);
																		add_assoc_string(files_entry, "tmp_name", tmp_file, 1);
																		add_assoc_long(files_entry, "error", 0);
																		add_assoc_long(files_entry, "size", current_data_len);
																}
																else
																{
																		add_assoc_null(files_entry, "type");
																		add_assoc_null(files_entry, "tmp_name");
																		add_assoc_long(files_entry, "error", 1);
																		add_assoc_long(files_entry, "size", 0);
																}

												
																add_assoc_zval(auto_global_files, data_name, files_entry);
																Z_ADDREF_P(files_entry);
																zend_hash_add(&EG(symbol_table), "_FILES", 7, &auto_global_files, sizeof(zval *), NULL);
																Z_ADDREF_P(auto_global_files);

														}
												}
										}
										//Data
										else
										{
												data_name = strstr(header, "name=\"");

												if(data_name != NULL)
												{
														data_name += 6; //bypass name="
														data_name_end = strstr(data_name, "\"");
														*data_name_end = 0; //ignore "\""

														long data_name_len = data_name_end - data_name;


                            int urlencode_len;
                            char * urlencode_data = php_url_encode(data, current_data_len, &urlencode_len);

                            if(input_parameter_string != NULL)
                            {
                                input_parameter_string_old = input_parameter_string;
                                //extra 3 characters are & = zero-terminal
                                input_parameter_string = malloc(strlen(input_parameter_string) + 1 + data_name_len + 1 + urlencode_len + 1);
                                sprintf( input_parameter_string, "%s&%s=%s", input_parameter_string_old, data_name, urlencode_data );
                                free(input_parameter_string_old);
                            }
                            else
                            {
                                //extra 2 characters are = zero-terminal
                                input_parameter_string = malloc(data_name_len + 1 + urlencode_len + 1);
                                sprintf( input_parameter_string, "%s=%s", data_name, urlencode_data );
                            }
													
												}
										}
								}


								if(current_input_data + current_part_len + boundary_len + 2 <= input_data_end)
								{
										current_input_data += current_part_len + boundary_len + 2; //shift to next part and bypass tail "\r\n"
										part_end = zend_memnstr(current_input_data, boundary, boundary_len, input_data_end);
								}
								else
								{
										break;
								}
								
						}

					
						if(input_parameter_string != NULL)
						{
								//Parse data
								MAKE_STD_ZVAL(parse_result);
								array_init(parse_result);

								char *input_parameter = estrndup(input_parameter_string, strlen(input_parameter_string));

								sapi_module.treat_data(PARSE_STRING, input_parameter, parse_result TSRMLS_CC);


								if( strcasecmp(Z_STRVAL_P(request_method), "put") == 0)
								{
										zend_hash_copy(Z_ARRVAL_P(put), Z_ARRVAL_P(parse_result), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
										zend_hash_add(&EG(symbol_table), "_PUT", 5, &put, sizeof(zval *), NULL);
								}
								else if( strcasecmp(Z_STRVAL_P(request_method), "delete") == 0)
								{
										zend_hash_copy(Z_ARRVAL_P(delete), Z_ARRVAL_P(parse_result), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
										zend_hash_add(&EG(symbol_table), "_DELETE", 8, &delete, sizeof(zval *), NULL);
								}

								free(input_parameter_string);
						}
				}

				free(input_data);
		}


		zval_ptr_dtor(&request_method);
		zval_ptr_dtor(&content_type);
}

zend_function_entry epv_functions[] = {
		{NULL, NULL, NULL}
};


#ifdef COMPILE_DL_EPV
ZEND_GET_MODULE(epv)
#endif

PHP_INI_BEGIN()
PHP_INI_END()

PHP_GINIT_FUNCTION(epv)
{
}



PHP_MINIT_FUNCTION(epv)
{
		epv_register_auto_global(TSRMLS_C);

		return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(epv)
{
		return SUCCESS;
}

PHP_RINIT_FUNCTION(epv)
{
		epv_init_auto_global(TSRMLS_C);

		return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(epv)
{
		return SUCCESS;
}

PHP_MINFO_FUNCTION(epv)
{
		php_info_print_table_start();
		php_info_print_table_colspan_header(2, "EPV (Extra Predefined Variables)");
		php_info_print_table_row(2, "Version", EPV_VERSION );
		php_info_print_table_end();

		DISPLAY_INI_ENTRIES();
}

static zend_module_dep epv_deps[] = {
		{NULL, NULL, NULL}
};

zend_module_entry epv_module_entry = {
		STANDARD_MODULE_HEADER_EX, 
		NULL,
		epv_deps,
		"epv",
		epv_functions,
		PHP_MINIT(epv),
		PHP_MSHUTDOWN(epv),
		PHP_RINIT(epv),
		PHP_RSHUTDOWN(epv),
		PHP_MINFO(epv),
		EPV_VERSION,
		PHP_MODULE_GLOBALS(epv),
		PHP_GINIT(epv),
		NULL,
		NULL,
		STANDARD_MODULE_PROPERTIES_EX
};
