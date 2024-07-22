PHP_MINIT_FUNCTION(dir)
{
	static char dirsep_str[2], pathsep_str[2];
	zend_class_entry dir_class_entry;

	INIT_CLASS_ENTRY(dir_class_entry, "Directory", php_dir_class_functions);
	dir_class_entry_ptr = zend_register_internal_class(&dir_class_entry TSRMLS_CC);

#ifdef ZTS
	ts_allocate_id(&dir_globals_id, sizeof(php_dir_globals), NULL, NULL);
#endif

	dirsep_str[0] = DEFAULT_SLASH;
	dirsep_str[1] = '\0';
	REGISTER_STRING_CONSTANT("DIRECTORY_SEPARATOR", dirsep_str, CONST_CS|CONST_PERSISTENT);

	pathsep_str[0] = ZEND_PATHS_SEPARATOR;
	pathsep_str[1] = '\0';
	REGISTER_STRING_CONSTANT("PATH_SEPARATOR", pathsep_str, CONST_CS|CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SCANDIR_SORT_ASCENDING",  PHP_SCANDIR_SORT_ASCENDING,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SCANDIR_SORT_DESCENDING", PHP_SCANDIR_SORT_DESCENDING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SCANDIR_SORT_NONE",       PHP_SCANDIR_SORT_NONE,       CONST_CS | CONST_PERSISTENT);

#ifdef HAVE_GLOB

#ifdef GLOB_BRACE
	REGISTER_LONG_CONSTANT("GLOB_BRACE", GLOB_BRACE, CONST_CS | CONST_PERSISTENT);
#else
# define GLOB_BRACE 0
#endif

#ifdef GLOB_MARK
	REGISTER_LONG_CONSTANT("GLOB_MARK", GLOB_MARK, CONST_CS | CONST_PERSISTENT);
#else
# define GLOB_MARK 0
#endif

#ifdef GLOB_NOSORT
	REGISTER_LONG_CONSTANT("GLOB_NOSORT", GLOB_NOSORT, CONST_CS | CONST_PERSISTENT);
#else 
# define GLOB_NOSORT 0
#endif

#ifdef GLOB_NOCHECK
	REGISTER_LONG_CONSTANT("GLOB_NOCHECK", GLOB_NOCHECK, CONST_CS | CONST_PERSISTENT);
#else 
# define GLOB_NOCHECK 0
#endif

#ifdef GLOB_NOESCAPE
	REGISTER_LONG_CONSTANT("GLOB_NOESCAPE", GLOB_NOESCAPE, CONST_CS | CONST_PERSISTENT);
#else 
# define GLOB_NOESCAPE 0
#endif

#ifdef GLOB_ERR
	REGISTER_LONG_CONSTANT("GLOB_ERR", GLOB_ERR, CONST_CS | CONST_PERSISTENT);
#else 
# define GLOB_ERR 0
#endif

#ifndef GLOB_ONLYDIR
# define GLOB_ONLYDIR (1<<30)
# define GLOB_EMULATE_ONLYDIR
# define GLOB_FLAGMASK (~GLOB_ONLYDIR)
#else
# define GLOB_FLAGMASK (~0)
#endif

/* This is used for checking validity of passed flags (passing invalid flags causes segfault in glob()!! */
#define GLOB_AVAILABLE_FLAGS (0 | GLOB_BRACE | GLOB_MARK | GLOB_NOSORT | GLOB_NOCHECK | GLOB_NOESCAPE | GLOB_ERR | GLOB_ONLYDIR)

	REGISTER_LONG_CONSTANT("GLOB_ONLYDIR", GLOB_ONLYDIR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GLOB_AVAILABLE_FLAGS", GLOB_AVAILABLE_FLAGS, CONST_CS | CONST_PERSISTENT);

#endif /* HAVE_GLOB */

	return SUCCESS;
}