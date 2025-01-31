read_system_page (Bit_Chain *dat, int64_t size_comp, int64_t size_uncomp,
                  int64_t repeat_count)
{
  int i;
  int error = 0;

  int64_t pesize;      // Pre RS encoded size
  int64_t block_count; // Number of RS encoded blocks
  int64_t page_size;
  long pedata_size;

  BITCODE_RC *rsdata; // RS encoded data
  BITCODE_RC *pedata; // Pre RS encoded data
  BITCODE_RC *data;   // The data RS unencoded and uncompressed

  if (repeat_count < 0 ||
      repeat_count > DBG_MAX_COUNT ||
      (uint64_t)size_comp >= dat->size ||
      (uint64_t)size_uncomp >= dat->size)
    {
      LOG_ERROR ("Invalid r2007 system page: "
                 "size_comp: %" PRId64 ", size_uncomp: %" PRId64
                 ", repeat_count: %" PRId64, size_comp, size_uncomp, repeat_count);
      return NULL;
    }
  // Round to a multiple of 8
  pesize = ((size_comp + 7) & ~7) * repeat_count;
  // Divide pre encoded size by RS k-value (239)
  block_count = (pesize + 238) / 239;
  if (block_count <= 0 || block_count > DBG_MAX_COUNT)
    {
      LOG_ERROR ("Invalid r2007 system page: size_comp: %" PRId64
                 ", size_uncomp: %" PRId64, size_comp, size_uncomp);
      return NULL;
    }
  // Multiply with codeword size (255) and round to a multiple of 8
  page_size = (block_count * 255 + 7) & ~7;
  if ((uint64_t)page_size >= DBG_MAX_COUNT || (unsigned long)page_size > dat->size - dat->byte)
    {
      LOG_ERROR ("Invalid r2007 system page: page_size: %" PRId64, page_size);
      return NULL;
    }
  assert ((uint64_t)size_comp < dat->size);
  assert ((uint64_t)size_uncomp < dat->size);
  assert ((uint64_t)repeat_count < DBG_MAX_COUNT);
  assert ((uint64_t)page_size < DBG_MAX_COUNT);
  data = (BITCODE_RC *)calloc (size_uncomp + page_size, 1);
  LOG_HANDLE ("Alloc system page of size %" PRId64 "\n", size_uncomp + page_size)
  if (!data)
    {
      LOG_ERROR ("Out of memory")
      return NULL;
    }

  rsdata = &data[size_uncomp];
  bit_read_fixed (dat, rsdata, page_size);
  pedata_size = block_count * 239;
  pedata = decode_rs (rsdata, block_count, 239, page_size);
  if (!pedata)
    {
      free (data);
      return NULL;
    }

  if (size_comp < size_uncomp)
    error = decompress_r2007 (data, size_uncomp, pedata, MIN (pedata_size, size_comp));
  else
    memcpy (data, pedata, size_uncomp);

  free (pedata);
  if (error >= DWG_ERR_CRITICAL)
    {
      free (data);
      return NULL;
    }
  return data;
}