#include "doctest/doctest.h"
#include "ra_common/messaging/command_message.hpp"
#include "ra_common/messaging/document_message.hpp"
#include "ra_common/messaging/message_json.hpp"

using namespace ra::common::messaging;

TEST_CASE("messaging tagged round trip") {
    {
        DocumentMessage doc;
        auto data = doc.ToJson();
        CHECK(data.contains("kind"));
        auto back = MessageFromJson(data);
        CHECK(dynamic_cast<DocumentMessage*>(back.get()) != nullptr);
    }
    {
        CommandMessage cmd(Command::Start);
        auto data = cmd.ToJson();
        CHECK(data.contains("kind"));
        auto back = MessageFromJson(data);
        CHECK(dynamic_cast<CommandMessage*>(back.get()) != nullptr);
    }
    CHECK(CommandMessage(Command::GracefullyShutdown).ToJson()["command"] == "GracefullyShutdown");
}
