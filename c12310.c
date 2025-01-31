TEST_CASE("Lambda action test", "[general]")
{
    parser parser(R"(
       START <- (CHAR)*
       CHAR  <- .
    )");

    std::string ss;
    parser["CHAR"] = [&](const SemanticValues& sv) {
        ss += *sv.c_str();
    };

    bool ret = parser.parse("hello");
    REQUIRE(ret == true);
    REQUIRE(ss == "hello");
}