static int bson_append_string_base( bson *b, const char *name,
                                    const char *value, int len, bson_type type ) {

    int sl = len + 1;
    if ( bson_check_string( b, ( const char * )value, sl - 1 ) == BSON_ERROR )
        return BSON_ERROR;
    if ( bson_append_estart( b, type, name, 4 + sl ) == BSON_ERROR ) {
        return BSON_ERROR;
    }
    bson_append32( b , &sl );
    bson_append( b , value , sl - 1 );
    bson_append( b , "\0" , 1 );
    return BSON_OK;
}