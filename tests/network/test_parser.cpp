
#include "network/RequestParser.hpp"
#include <gtest/gtest.h>

using namespace oops;

TEST(RequestParserTest, ValidGet) {
    Request request = parseRequest("GET username");

    EXPECT_EQ(request.command, Command::Get);
    EXPECT_EQ(request.key, "username");
}

TEST(RequestParserTest, MissingGetKey) {
    Request request = parseRequest("GET");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, ExtraGetArgument) {
    Request request = parseRequest("GET username extra");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, MultiwordSet) {
    Request request = parseRequest("SET greeting hello world");

    EXPECT_EQ(request.command, Command::Set);
    EXPECT_EQ(request.key, "greeting");
    EXPECT_EQ(request.value, "hello world");
}

TEST(RequestParserTest, MissingSetValue) {
    Request request = parseRequest("SET greeting");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, LongerSetValue) {
    Request request = parseRequest("SET message this is a longer value");

    EXPECT_EQ(request.command, Command::Set);
    EXPECT_EQ(request.key, "message");
    EXPECT_EQ(request.value, "this is a longer value");
}

TEST(RequestParserTest, ValidGetVersion) {
    Request request = parseRequest("GET_VERSION colour 2");

    EXPECT_EQ(request.command, Command::GetVersion);
    EXPECT_EQ(request.key, "colour");
    EXPECT_EQ(request.version, 2);
}

TEST(RequestParserTest, MalformedVersion) {
    Request request = parseRequest("GET_VERSION colour abc");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, NegativeVersion) {
    Request request = parseRequest("GET_VERSION colour -1");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, VersionOverflow) {
    Request request = parseRequest(
        "GET_VERSION colour 18446744073709551616"
    );

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, MissingVersion) {
    Request request = parseRequest("GET_VERSION colour");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, ExtraVersionArgument) {
    Request request = parseRequest("GET_VERSION colour 2 extra");

    EXPECT_EQ(request.command, Command::Invalid);
}

TEST(RequestParserTest, ValidPing) {
    Request request = parseRequest("PING");

    EXPECT_EQ(request.command, Command::Ping);
}

TEST(RequestParserTest, ExtraPingArgument) {
    Request request = parseRequest("PING extra");

    EXPECT_EQ(request.command, Command::Invalid);
}
