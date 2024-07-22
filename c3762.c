static input_translation_t *add_translation_table(exporter_ipfix_domain_t *exporter, uint16_t id) {
input_translation_t **table;

	table = &(exporter->input_translation_table);
	while ( *table ) {
		table = &((*table)->next);
	}

	// Allocate enough space for all potential ipfix tags, which we support
	// so template refreshing may change the table size without danger of overflowing 
	*table = calloc(1, sizeof(input_translation_t));
	if ( !(*table) ) {
			syslog(LOG_ERR, "Process_ipfix: Panic! calloc() %s line %d: %s", __FILE__, __LINE__, strerror (errno));
			return NULL;
	}
	(*table)->sequence = calloc(cache.max_ipfix_elements, sizeof(sequence_map_t));
	if ( !(*table)->sequence ) {
			syslog(LOG_ERR, "Process_ipfix: Panic! malloc() %s line %d: %s", __FILE__, __LINE__, strerror (errno));
			return NULL;
	}

	(*table)->id   = id;
	(*table)->next = NULL;

	dbg_printf("[%u] Get new translation table %u\n", exporter->info.id, id);

	return *table;

} // End of add_translation_table