static int fts3IncrmergePush(
  Fts3Table *p,                   /* Fts3 table handle */
  IncrmergeWriter *pWriter,       /* Writer object */
  const char *zTerm,              /* Term to write to internal node */
  int nTerm                       /* Bytes at zTerm */
){
  sqlite3_int64 iPtr = pWriter->aNodeWriter[0].iBlock;
  int iLayer;

  assert( nTerm>0 );
  for(iLayer=1; ALWAYS(iLayer<FTS_MAX_APPENDABLE_HEIGHT); iLayer++){
    sqlite3_int64 iNextPtr = 0;
    NodeWriter *pNode = &pWriter->aNodeWriter[iLayer];
    int rc = SQLITE_OK;
    int nPrefix;
    int nSuffix;
    int nSpace;

    /* Figure out how much space the key will consume if it is written to
    ** the current node of layer iLayer. Due to the prefix compression, 
    ** the space required changes depending on which node the key is to
    ** be added to.  */
    nPrefix = fts3PrefixCompress(pNode->key.a, pNode->key.n, zTerm, nTerm);
    nSuffix = nTerm - nPrefix;
    nSpace  = sqlite3Fts3VarintLen(nPrefix);
    nSpace += sqlite3Fts3VarintLen(nSuffix) + nSuffix;

    if( pNode->key.n==0 || (pNode->block.n + nSpace)<=p->nNodeSize ){ 
      /* If the current node of layer iLayer contains zero keys, or if adding
      ** the key to it will not cause it to grow to larger than nNodeSize 
      ** bytes in size, write the key here.  */

      Blob *pBlk = &pNode->block;
      if( pBlk->n==0 ){
        blobGrowBuffer(pBlk, p->nNodeSize, &rc);
        if( rc==SQLITE_OK ){
          pBlk->a[0] = (char)iLayer;
          pBlk->n = 1 + sqlite3Fts3PutVarint(&pBlk->a[1], iPtr);
        }
      }
      blobGrowBuffer(pBlk, pBlk->n + nSpace, &rc);
      blobGrowBuffer(&pNode->key, nTerm, &rc);

      if( rc==SQLITE_OK ){
        if( pNode->key.n ){
          pBlk->n += sqlite3Fts3PutVarint(&pBlk->a[pBlk->n], nPrefix);
        }
        pBlk->n += sqlite3Fts3PutVarint(&pBlk->a[pBlk->n], nSuffix);
        memcpy(&pBlk->a[pBlk->n], &zTerm[nPrefix], nSuffix);
        pBlk->n += nSuffix;

        memcpy(pNode->key.a, zTerm, nTerm);
        pNode->key.n = nTerm;
      }
    }else{
      /* Otherwise, flush the current node of layer iLayer to disk.
      ** Then allocate a new, empty sibling node. The key will be written
      ** into the parent of this node. */
      rc = fts3WriteSegment(p, pNode->iBlock, pNode->block.a, pNode->block.n);

      assert( pNode->block.nAlloc>=p->nNodeSize );
      pNode->block.a[0] = (char)iLayer;
      pNode->block.n = 1 + sqlite3Fts3PutVarint(&pNode->block.a[1], iPtr+1);

      iNextPtr = pNode->iBlock;
      pNode->iBlock++;
      pNode->key.n = 0;
    }

    if( rc!=SQLITE_OK || iNextPtr==0 ) return rc;
    iPtr = iNextPtr;
  }

  assert( 0 );
  return 0;
}