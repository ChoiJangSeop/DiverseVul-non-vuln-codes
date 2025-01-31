static PHP_MINIT_FUNCTION(zip)
{
#ifdef PHP_ZIP_USE_OO 
	zend_class_entry ce;

	memcpy(&zip_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zip_object_handlers.clone_obj		= NULL;
	zip_object_handlers.get_property_ptr_ptr = php_zip_get_property_ptr_ptr;

	zip_object_handlers.get_properties = php_zip_get_properties;
	zip_object_handlers.read_property	= php_zip_read_property;
	zip_object_handlers.has_property	= php_zip_has_property;

	INIT_CLASS_ENTRY(ce, "ZipArchive", zip_class_functions);
	ce.create_object = php_zip_object_new;
	zip_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	zend_hash_init(&zip_prop_handlers, 0, NULL, NULL, 1);
	php_zip_register_prop_handler(&zip_prop_handlers, "status",    php_zip_status, NULL, NULL, IS_LONG TSRMLS_CC);
	php_zip_register_prop_handler(&zip_prop_handlers, "statusSys", php_zip_status_sys, NULL, NULL, IS_LONG TSRMLS_CC);
	php_zip_register_prop_handler(&zip_prop_handlers, "numFiles",  php_zip_get_num_files, NULL, NULL, IS_LONG TSRMLS_CC);
	php_zip_register_prop_handler(&zip_prop_handlers, "filename", NULL, NULL, php_zipobj_get_filename, IS_STRING TSRMLS_CC);
	php_zip_register_prop_handler(&zip_prop_handlers, "comment", NULL, php_zipobj_get_zip_comment, NULL, IS_STRING TSRMLS_CC);

	REGISTER_ZIP_CLASS_CONST_LONG("CREATE", ZIP_CREATE);
	REGISTER_ZIP_CLASS_CONST_LONG("EXCL", ZIP_EXCL);
	REGISTER_ZIP_CLASS_CONST_LONG("CHECKCONS", ZIP_CHECKCONS);
	REGISTER_ZIP_CLASS_CONST_LONG("OVERWRITE", ZIP_OVERWRITE);

	REGISTER_ZIP_CLASS_CONST_LONG("FL_NOCASE", ZIP_FL_NOCASE);
	REGISTER_ZIP_CLASS_CONST_LONG("FL_NODIR", ZIP_FL_NODIR);
	REGISTER_ZIP_CLASS_CONST_LONG("FL_COMPRESSED", ZIP_FL_COMPRESSED);
	REGISTER_ZIP_CLASS_CONST_LONG("FL_UNCHANGED", ZIP_FL_UNCHANGED);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_DEFAULT", ZIP_CM_DEFAULT);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_STORE", ZIP_CM_STORE);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_SHRINK", ZIP_CM_SHRINK);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_REDUCE_1", ZIP_CM_REDUCE_1);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_REDUCE_2", ZIP_CM_REDUCE_2);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_REDUCE_3", ZIP_CM_REDUCE_3);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_REDUCE_4", ZIP_CM_REDUCE_4);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_IMPLODE", ZIP_CM_IMPLODE);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_DEFLATE", ZIP_CM_DEFLATE);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_DEFLATE64", ZIP_CM_DEFLATE64);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_PKWARE_IMPLODE", ZIP_CM_PKWARE_IMPLODE);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_BZIP2", ZIP_CM_BZIP2);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_LZMA", ZIP_CM_LZMA);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_TERSE", ZIP_CM_TERSE);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_LZ77", ZIP_CM_LZ77);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_WAVPACK", ZIP_CM_WAVPACK);
	REGISTER_ZIP_CLASS_CONST_LONG("CM_PPMD", ZIP_CM_PPMD);

	/* Error code */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_OK",			ZIP_ER_OK);			/* N No error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_MULTIDISK",	ZIP_ER_MULTIDISK);	/* N Multi-disk zip archives not supported */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_RENAME",		ZIP_ER_RENAME);		/* S Renaming temporary file failed */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_CLOSE",		ZIP_ER_CLOSE);		/* S Closing zip archive failed */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_SEEK",		ZIP_ER_SEEK);		/* S Seek error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_READ",		ZIP_ER_READ);		/* S Read error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_WRITE",		ZIP_ER_WRITE);		/* S Write error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_CRC",			ZIP_ER_CRC);		/* N CRC error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_ZIPCLOSED",	ZIP_ER_ZIPCLOSED);	/* N Containing zip archive was closed */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_NOENT",		ZIP_ER_NOENT);		/* N No such file */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_EXISTS",		ZIP_ER_EXISTS);		/* N File already exists */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_OPEN",		ZIP_ER_OPEN);		/* S Can't open file */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_TMPOPEN",		ZIP_ER_TMPOPEN);	/* S Failure to create temporary file */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_ZLIB",		ZIP_ER_ZLIB);		/* Z Zlib error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_MEMORY",		ZIP_ER_MEMORY);		/* N Malloc failure */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_CHANGED",		ZIP_ER_CHANGED);	/* N Entry has been changed */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_COMPNOTSUPP",	ZIP_ER_COMPNOTSUPP);/* N Compression method not supported */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_EOF",			ZIP_ER_EOF);		/* N Premature EOF */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_INVAL",		ZIP_ER_INVAL);		/* N Invalid argument */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_NOZIP",		ZIP_ER_NOZIP);		/* N Not a zip archive */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_INTERNAL",	ZIP_ER_INTERNAL);	/* N Internal error */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_INCONS",		ZIP_ER_INCONS);		/* N Zip archive inconsistent */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_REMOVE",		ZIP_ER_REMOVE);		/* S Can't remove file */
	REGISTER_ZIP_CLASS_CONST_LONG("ER_DELETED",  	ZIP_ER_DELETED);	/* N Entry has been deleted */

	php_register_url_stream_wrapper("zip", &php_stream_zip_wrapper TSRMLS_CC);
#endif

	le_zip_dir   = zend_register_list_destructors_ex(php_zip_free_dir,   NULL, le_zip_dir_name,   module_number);
	le_zip_entry = zend_register_list_destructors_ex(php_zip_free_entry, NULL, le_zip_entry_name, module_number);

	return SUCCESS;
}