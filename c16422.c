dns_enqueue(const char *name, size_t hostnamelen, dns_found_callback found,
            void *callback_arg)
{
  u8_t i;
  u8_t lseq, lseqi;
  struct dns_table_entry *entry = NULL;
  size_t namelen;
  struct dns_req_entry* req;

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  u8_t r;
  /* check for duplicate entries */
  for (i = 0; i < DNS_TABLE_SIZE; i++) {
    if ((dns_table[i].state == DNS_STATE_ASKING) &&
        (LWIP_DNS_STRICMP(name, dns_table[i].name) == 0)) {
      /* this is a duplicate entry, find a free request entry */
      for (r = 0; r < DNS_MAX_REQUESTS; r++) {
        if (dns_requests[r].found == 0) {
          dns_requests[r].found = found;
          dns_requests[r].arg = callback_arg;
          dns_requests[r].dns_table_idx = i;
          LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": duplicate request\n", name));
          return ERR_INPROGRESS;
        }
      }
    }
  }
  /* no duplicate entries found */
#endif

  /* search an unused entry, or the oldest one */
  lseq = 0;
  lseqi = DNS_TABLE_SIZE;
  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    entry = &dns_table[i];
    /* is it an unused entry ? */
    if (entry->state == DNS_STATE_UNUSED) {
      break;
    }
    /* check if this is the oldest completed entry */
    if (entry->state == DNS_STATE_DONE) {
      if ((dns_seqno - entry->seqno) > lseq) {
        lseq = dns_seqno - entry->seqno;
        lseqi = i;
      }
    }
  }

  /* if we don't have found an unused entry, use the oldest completed one */
  if (i == DNS_TABLE_SIZE) {
    if ((lseqi >= DNS_TABLE_SIZE) || (dns_table[lseqi].state != DNS_STATE_DONE)) {
      /* no entry can be used now, table is full */
      LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": DNS entries table is full\n", name));
      return ERR_MEM;
    } else {
      /* use the oldest completed one */
      i = lseqi;
      entry = &dns_table[i];
    }
  }

#if ((LWIP_DNS_SECURE & LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING) != 0)
  /* find a free request entry */
  req = NULL;
  for (r = 0; r < DNS_MAX_REQUESTS; r++) {
    if (dns_requests[r].found == 0) {
      req = &dns_requests[r];
      break;
    }
  }
  if (req == NULL) {
    /* no request entry can be used now, table is full */
    LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": DNS request entries table is full\n", name));
    return ERR_MEM;
  }
#else
  /* in this configuration, the entry index is the same as the request index */
  req = &dns_requests[i];
#endif


  /* use this entry */
  LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": use DNS entry %"U16_F"\n", name, (u16_t)(i)));

  /* fill the entry */
  entry->state = DNS_STATE_NEW;
  entry->seqno = dns_seqno;
  req->found = found;
  req->arg   = callback_arg;
  namelen = LWIP_MIN(hostnamelen, DNS_MAX_NAME_LENGTH-1);
  MEMCPY(entry->name, name, namelen);
  entry->name[namelen] = 0;

  dns_seqno++;

  /* force to send query without waiting timer */
  dns_check_entry(i);

  /* dns query is enqueued */
  return ERR_INPROGRESS;
}