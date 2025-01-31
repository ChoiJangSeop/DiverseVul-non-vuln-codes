hash_V5(std::string const& password,
        std::string const& salt,
        std::string const& udata,
        QPDF::EncryptionData const& data)
{
    Pl_SHA2 hash(256);
    hash.write(QUtil::unsigned_char_pointer(password), password.length());
    hash.write(QUtil::unsigned_char_pointer(salt), salt.length());
    hash.write(QUtil::unsigned_char_pointer(udata), udata.length());
    hash.finish();
    std::string K = hash.getRawDigest();

    std::string result;
    if (data.getR() < 6)
    {
        result = K;
    }
    else
    {
        // Algorithm 2.B from ISO 32000-1 chapter 7: Computing a hash

        int round_number = 0;
        bool done = false;
        while (! done)
        {
            // The hash algorithm has us setting K initially to the R5
            // value and then repeating a series of steps 64 times
            // before starting with the termination case testing.  The
            // wording of the specification is very unclear as to the
            // exact number of times it should be run since the
            // wording about whether the initial setup counts as round
            // 0 or not is ambiguous.  This code counts the initial
            // setup (R5) value as round 0, which appears to be
            // correct.  This was determined to be correct by
            // increasing or decreasing the number of rounds by 1 or 2
            // from this value and generating 20 test files.  In this
            // interpretation, all the test files worked with Adobe
            // Reader X.  In the other configurations, many of the
            // files did not work, and we were accurately able to
            // predict which files didn't work by looking at the
            // conditions under which we terminated repetition.

            ++round_number;
            std::string K1 = password + K + udata;
            assert(K.length() >= 32);
            std::string E = process_with_aes(
                K.substr(0, 16), true, K1, 0, 64,
                QUtil::unsigned_char_pointer(K.substr(16, 16)), 16);

            // E_mod_3 is supposed to be mod 3 of the first 16 bytes
            // of E taken as as a (128-bit) big-endian number.  Since
            // (xy mod n) is equal to ((x mod n) + (y mod n)) mod n
            // and since 256 mod n is 1, we can just take the sums of
            // the the mod 3s of each byte to get the same result.
            int E_mod_3 = 0;
            for (unsigned int i = 0; i < 16; ++i)
            {
                E_mod_3 += static_cast<unsigned char>(E.at(i));
            }
            E_mod_3 %= 3;
            int next_hash = ((E_mod_3 == 0) ? 256 :
                             (E_mod_3 == 1) ? 384 :
                             512);
            Pl_SHA2 hash(next_hash);
            hash.write(QUtil::unsigned_char_pointer(E), E.length());
            hash.finish();
            K = hash.getRawDigest();

            if (round_number >= 64)
            {
                unsigned int ch = static_cast<unsigned char>(*(E.rbegin()));

                if (ch <= static_cast<unsigned int>(round_number - 32))
                {
                    done = true;
                }
            }
        }
        result = K.substr(0, 32);
    }

    return result;
}