SecureRandomDataProvider::provideRandomData(unsigned char* data, size_t len)
{
#if defined(_WIN32)

    // Optimization: make the WindowsCryptProvider static as long as
    // it can be done in a thread-safe fashion.
    WindowsCryptProvider c;
    if (! CryptGenRandom(c.crypt_prov, len, reinterpret_cast<BYTE*>(data)))
    {
        throw std::runtime_error("unable to generate secure random data");
    }

#elif defined(RANDOM_DEVICE)

    // Optimization: wrap the file open and close in a class so that
    // the file is closed in a destructor, then make this static to
    // keep the file handle open.  Only do this if it can be done in a
    // thread-safe fashion.
    FILE* f = QUtil::safe_fopen(RANDOM_DEVICE, "rb");
    size_t fr = fread(data, 1, len, f);
    fclose(f);
    if (fr != len)
    {
        throw std::runtime_error(
            "unable to read " +
            QUtil::int_to_string(len) +
            " bytes from " + std::string(RANDOM_DEVICE));
    }

#else

#  error "Don't know how to generate secure random numbers on this platform.  See random number generation in the top-level README.md"

#endif
}