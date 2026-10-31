#include "network/ResponseSerializer.hpp"
#include <gtest/gtest.h>

using namespace oops;

TEST(ResponseSerializerTest, ValueTakesPrecedenceOverMessage) {
    Response response;
    response.status = ResponseStatus::Success;
    response.message = "Key retrieved successfully";
    response.value = "hello world";

    EXPECT_EQ(serializeResponse(response), "OK hello world\n");
}

TEST(ResponseSerializerTest, MessageUsedWhenValueIsEmpty) {
    Response response;
    response.status = ResponseStatus::Success;
    response.message = "PONG";

    EXPECT_EQ(serializeResponse(response), "OK PONG\n");
}