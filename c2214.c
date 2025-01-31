void XMLRPC_SetValueDateTime_ISO8601(XMLRPC_VALUE value, const char* s) {
   if(value) {
      time_t time_val = 0;
      if(s) {
         date_from_ISO8601(s, &time_val);
         XMLRPC_SetValueDateTime(value, time_val);
      }
   }
}