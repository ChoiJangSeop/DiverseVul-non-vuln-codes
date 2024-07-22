void SetErrorString(YasslError error, char* buffer)
{
    using namespace TaoCrypt;
    const int max = MAX_ERROR_SZ;  // shorthand
    int localError = error;        // errors from a few enums 

    switch (localError) {

        // yaSSL proper errors
    case range_error :
        strcpy_s(buffer, max, "buffer index error, out of range");
        break; 

    case realloc_error :
        strcpy_s(buffer, max, "trying to realloc a fixed buffer");
        break; 

    case factory_error : 
        strcpy_s(buffer, max, "unknown factory create request");
        break; 

    case unknown_cipher :
        strcpy_s(buffer, max, "trying to use an unknown cipher");
        break; 

    case prefix_error : 
        strcpy_s(buffer, max, "bad master secret derivation, prefix too big");
        break; 

    case record_layer : 
        strcpy_s(buffer, max, "record layer not ready yet");
        break; 
        
    case handshake_layer :
        strcpy_s(buffer, max, "handshake layer not ready yet");
        break; 

    case out_of_order :
        strcpy_s(buffer, max, "handshake message received in wrong order");
        break; 

    case bad_input : 
        strcpy_s(buffer, max, "bad cipher suite input");
        break; 

    case match_error :
        strcpy_s(buffer, max, "unable to match a supported cipher suite");
        break; 

    case no_key_file : 
        strcpy_s(buffer, max, "the server needs a private key file");
        break; 

    case verify_error :
        strcpy_s(buffer, max, "unable to verify peer checksum");
        break; 

    case send_error :
        strcpy_s(buffer, max, "socket layer send error");
        break; 

    case receive_error :
        strcpy_s(buffer, max, "socket layer receive error");
        break; 

    case certificate_error :
        strcpy_s(buffer, max, "unable to proccess cerificate");
        break;

    case privateKey_error :
        strcpy_s(buffer, max, "unable to proccess private key, bad format");
        break;

    case badVersion_error :
        strcpy_s(buffer, max, "protocol version mismatch");
        break;

    case compress_error :
        strcpy_s(buffer, max, "compression error");
        break;

    case decompress_error :
        strcpy_s(buffer, max, "decompression error");
        break;

    case pms_version_error :
        strcpy_s(buffer, max, "bad PreMasterSecret version error");
        break;

    case sanityCipher_error :
        strcpy_s(buffer, max, "sanity check on cipher text size error");
        break;

        // openssl errors
    case SSL_ERROR_WANT_READ :
        strcpy_s(buffer, max, "the read operation would block");
        break;

    case SSL_ERROR_WANT_WRITE :
        strcpy_s(buffer, max, "the write operation would block");
        break;

    case CERTFICATE_ERROR :
        strcpy_s(buffer, max, "Unable to verify certificate");
        break;

        // TaoCrypt errors
    case NO_ERROR_E :
        strcpy_s(buffer, max, "not in error state");
        break;

    case WINCRYPT_E :
        strcpy_s(buffer, max, "bad wincrypt acquire");
        break;

    case CRYPTGEN_E :
        strcpy_s(buffer, max, "CryptGenRandom error");
        break;

    case OPEN_RAN_E :
        strcpy_s(buffer, max, "unable to use random device");
        break;

    case READ_RAN_E :
        strcpy_s(buffer, max, "unable to use random device");
        break;

    case INTEGER_E :
        strcpy_s(buffer, max, "ASN: bad DER Integer Header");
        break;

    case SEQUENCE_E :
        strcpy_s(buffer, max, "ASN: bad Sequence Header");
        break;

    case SET_E :
        strcpy_s(buffer, max, "ASN: bad Set Header");
        break;

    case VERSION_E :
        strcpy_s(buffer, max, "ASN: version length not 1");
        break;

    case SIG_OID_E :
        strcpy_s(buffer, max, "ASN: signature OID mismatch");
        break;

    case BIT_STR_E :
        strcpy_s(buffer, max, "ASN: bad BitString Header");
        break;

    case UNKNOWN_OID_E :
        strcpy_s(buffer, max, "ASN: unknown key OID type");
        break;

    case OBJECT_ID_E :
        strcpy_s(buffer, max, "ASN: bad Ojbect ID Header");
        break;

    case TAG_NULL_E :
        strcpy_s(buffer, max, "ASN: expected TAG NULL");
        break;

    case EXPECT_0_E :
        strcpy_s(buffer, max, "ASN: expected 0");
        break;

    case OCTET_STR_E :
        strcpy_s(buffer, max, "ASN: bad Octet String Header");
        break;

    case TIME_E :
        strcpy_s(buffer, max, "ASN: bad TIME");
        break;

    case DATE_SZ_E :
        strcpy_s(buffer, max, "ASN: bad Date Size");
        break;

    case SIG_LEN_E :
        strcpy_s(buffer, max, "ASN: bad Signature Length");
        break;

    case UNKOWN_SIG_E :
        strcpy_s(buffer, max, "ASN: unknown signature OID");
        break;

    case UNKOWN_HASH_E :
        strcpy_s(buffer, max, "ASN: unknown hash OID");
        break;

    case DSA_SZ_E :
        strcpy_s(buffer, max, "ASN: bad DSA r or s size");
        break;

    case BEFORE_DATE_E :
        strcpy_s(buffer, max, "ASN: before date in the future");
        break;

    case AFTER_DATE_E :
        strcpy_s(buffer, max, "ASN: after date in the past");
        break;

    case SIG_CONFIRM_E :
        strcpy_s(buffer, max, "ASN: bad self signature confirmation");
        break;

    case SIG_OTHER_E :
        strcpy_s(buffer, max, "ASN: bad other signature confirmation");
        break;

    case CONTENT_E :
        strcpy_s(buffer, max, "bad content processing");
        break;

    case PEM_E :
        strcpy_s(buffer, max, "bad PEM format processing");
        break;

    default :
        strcpy_s(buffer, max, "unknown error number");
    }
}