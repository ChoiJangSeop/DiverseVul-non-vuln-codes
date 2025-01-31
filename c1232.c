void init_directory_config(directory_config *dcfg)
{
    if (dcfg == NULL) return;

    if (dcfg->is_enabled == NOT_SET) dcfg->is_enabled = 0;

    if (dcfg->reqbody_access == NOT_SET) dcfg->reqbody_access = 0;
    if (dcfg->reqintercept_oe == NOT_SET) dcfg->reqintercept_oe = 0;
    if (dcfg->reqbody_buffering == NOT_SET) dcfg->reqbody_buffering = REQUEST_BODY_FORCEBUF_OFF;
    if (dcfg->reqbody_inmemory_limit == NOT_SET)
        dcfg->reqbody_inmemory_limit = REQUEST_BODY_DEFAULT_INMEMORY_LIMIT;
    if (dcfg->reqbody_limit == NOT_SET) dcfg->reqbody_limit = REQUEST_BODY_DEFAULT_LIMIT;
    if (dcfg->reqbody_no_files_limit == NOT_SET) dcfg->reqbody_no_files_limit = REQUEST_BODY_NO_FILES_DEFAULT_LIMIT;
    if (dcfg->resbody_access == NOT_SET) dcfg->resbody_access = 0;
    if (dcfg->of_limit == NOT_SET) dcfg->of_limit = RESPONSE_BODY_DEFAULT_LIMIT;
    if (dcfg->if_limit_action == NOT_SET) dcfg->if_limit_action = REQUEST_BODY_LIMIT_ACTION_REJECT;
    if (dcfg->of_limit_action == NOT_SET) dcfg->of_limit_action = RESPONSE_BODY_LIMIT_ACTION_REJECT;

    if (dcfg->of_mime_types == NOT_SET_P) {
        dcfg->of_mime_types = apr_table_make(dcfg->mp, 3);
        if (dcfg->of_mime_types_cleared != 1) {
            apr_table_setn(dcfg->of_mime_types, "text/plain", "1");
            apr_table_setn(dcfg->of_mime_types, "text/html", "1");
        }
    }

    if (dcfg->debuglog_fd == NOT_SET_P) dcfg->debuglog_fd = NULL;
    if (dcfg->debuglog_name == NOT_SET_P) dcfg->debuglog_name = NULL;
    if (dcfg->debuglog_level == NOT_SET) dcfg->debuglog_level = 0;

    if (dcfg->cookie_format == NOT_SET) dcfg->cookie_format = 0;
    if (dcfg->argument_separator == NOT_SET) dcfg->argument_separator = '&';
    if (dcfg->cookiev0_separator == NOT_SET_P) dcfg->cookiev0_separator = NULL;

    if (dcfg->rule_inheritance == NOT_SET) dcfg->rule_inheritance = 1;

    /* audit log variables */
    if (dcfg->auditlog_flag == NOT_SET) dcfg->auditlog_flag = 0;
    if (dcfg->auditlog_type == NOT_SET) dcfg->auditlog_type = AUDITLOG_SERIAL;
    if (dcfg->max_rule_time == NOT_SET) dcfg->max_rule_time = 0;
    if (dcfg->auditlog_dirperms == NOT_SET) dcfg->auditlog_dirperms = CREATEMODE_DIR;
    if (dcfg->auditlog_fileperms == NOT_SET) dcfg->auditlog_fileperms = CREATEMODE;
    if (dcfg->auditlog_fd == NOT_SET_P) dcfg->auditlog_fd = NULL;
    if (dcfg->auditlog2_fd == NOT_SET_P) dcfg->auditlog2_fd = NULL;
    if (dcfg->auditlog_name == NOT_SET_P) dcfg->auditlog_name = NULL;
    if (dcfg->auditlog2_name == NOT_SET_P) dcfg->auditlog2_name = NULL;
    if (dcfg->auditlog_storage_dir == NOT_SET_P) dcfg->auditlog_storage_dir = NULL;
    if (dcfg->auditlog_parts == NOT_SET_P) dcfg->auditlog_parts = "ABCFHZ";
    if (dcfg->auditlog_relevant_regex == NOT_SET_P) dcfg->auditlog_relevant_regex = NULL;

    /* Upload */
    if (dcfg->tmp_dir == NOT_SET_P) dcfg->tmp_dir = guess_tmp_dir(dcfg->mp);
    if (dcfg->upload_dir == NOT_SET_P) dcfg->upload_dir = NULL;
    if (dcfg->upload_keep_files == NOT_SET) dcfg->upload_keep_files = KEEP_FILES_OFF;
    if (dcfg->upload_validates_files == NOT_SET) dcfg->upload_validates_files = 0;
    if (dcfg->upload_filemode == NOT_SET) dcfg->upload_filemode = 0600;
    if (dcfg->upload_file_limit == NOT_SET) dcfg->upload_file_limit = 100;

    /* Misc */
    if (dcfg->data_dir == NOT_SET_P) dcfg->data_dir = NULL;
    if (dcfg->webappid == NOT_SET_P) dcfg->webappid = "default";
    if (dcfg->sensor_id == NOT_SET_P) dcfg->sensor_id = "default";
    if (dcfg->httpBlkey == NOT_SET_P) dcfg->httpBlkey = NULL;

    /* Content injection. */
    if (dcfg->content_injection_enabled == NOT_SET) dcfg->content_injection_enabled = 0;

    /* Stream inspection */
    if (dcfg->stream_inbody_inspection == NOT_SET) dcfg->stream_inbody_inspection = 0;
    if (dcfg->stream_outbody_inspection == NOT_SET) dcfg->stream_outbody_inspection = 0;

    /* Geo Lookup */
    if (dcfg->geo == NOT_SET_P) dcfg->geo = NULL;

    /* Gsb Lookup */
    if (dcfg->gsb == NOT_SET_P) dcfg->gsb = NULL;

    /* Unicode Map */
    if (dcfg->u_map == NOT_SET_P) dcfg->u_map = NULL;

    /* Cache */
    if (dcfg->cache_trans == NOT_SET) dcfg->cache_trans = MODSEC_CACHE_DISABLED;
    if (dcfg->cache_trans_incremental == NOT_SET) dcfg->cache_trans_incremental = 0;
    if (dcfg->cache_trans_min == (apr_size_t)NOT_SET) dcfg->cache_trans_min = 32;
    if (dcfg->cache_trans_max == (apr_size_t)NOT_SET) dcfg->cache_trans_max = 1024;
    if (dcfg->cache_trans_maxitems == (apr_size_t)NOT_SET) dcfg->cache_trans_maxitems = 512;

    if (dcfg->request_encoding == NOT_SET_P) dcfg->request_encoding = NULL;

    if (dcfg->disable_backend_compression == NOT_SET) dcfg->disable_backend_compression = 0;

    if (dcfg->col_timeout == NOT_SET) dcfg->col_timeout = 3600;

    /* Hash */
    if (dcfg->crypto_key == NOT_SET_P) dcfg->crypto_key = getkey(dcfg->mp);
    if (dcfg->crypto_key_len == NOT_SET) dcfg->crypto_key_len = strlen(dcfg->crypto_key);
    if (dcfg->crypto_key_add == NOT_SET) dcfg->crypto_key_add = HASH_KEYONLY;
    if (dcfg->crypto_param_name == NOT_SET_P) dcfg->crypto_param_name = "crypt";
    if (dcfg->hash_is_enabled == NOT_SET) dcfg->hash_is_enabled = HASH_DISABLED;
    if (dcfg->hash_enforcement == NOT_SET) dcfg->hash_enforcement = HASH_DISABLED;
    if (dcfg->crypto_hash_href_rx == NOT_SET) dcfg->crypto_hash_href_rx = 0;
    if (dcfg->crypto_hash_faction_rx == NOT_SET) dcfg->crypto_hash_faction_rx = 0;
    if (dcfg->crypto_hash_location_rx == NOT_SET) dcfg->crypto_hash_location_rx = 0;
    if (dcfg->crypto_hash_iframesrc_rx == NOT_SET) dcfg->crypto_hash_iframesrc_rx = 0;
    if (dcfg->crypto_hash_framesrc_rx == NOT_SET) dcfg->crypto_hash_framesrc_rx = 0;
    if (dcfg->crypto_hash_href_pm == NOT_SET) dcfg->crypto_hash_href_pm = 0;
    if (dcfg->crypto_hash_faction_pm == NOT_SET) dcfg->crypto_hash_faction_pm = 0;
    if (dcfg->crypto_hash_location_pm == NOT_SET) dcfg->crypto_hash_location_pm = 0;
    if (dcfg->crypto_hash_iframesrc_pm == NOT_SET) dcfg->crypto_hash_iframesrc_pm = 0;
    if (dcfg->crypto_hash_framesrc_pm == NOT_SET) dcfg->crypto_hash_framesrc_pm = 0;

}