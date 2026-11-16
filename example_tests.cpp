#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "jackal_extension.h"
#include <nlohmann/json.hpp>

extern "C" int __cdecl ExampleCommand(const char*, char*, uint32_t, uint32_t*);
extern "C" int __cdecl ExampleCommandHook(const char*, char*, uint32_t, uint32_t*);

namespace {
using nlohmann::json;

using Handler = int(__cdecl*)(const char*, char*, uint32_t, uint32_t*);

json Invoke(Handler handler, const json& request) {
    const auto input = request.dump();
    std::vector<char> buffer(4096);
    uint32_t required = 0;
    int status = handler(input.c_str(), buffer.data(), static_cast<uint32_t>(buffer.size()), &required);
    if (status == JC_EXTENSION_BUFFER_TOO_SMALL) {
        buffer.resize(required);
        status = handler(input.c_str(), buffer.data(), static_cast<uint32_t>(buffer.size()), &required);
    }
    assert(status == JC_EXTENSION_OK);
    return json::parse(buffer.data());
}

void TestCommands() {
    const auto echo = Invoke(ExampleCommand, {
        { "path", "/example echo" }, { "arguments", { { "text", "hello world" } } }
    });
    assert(echo.at("ok").get<bool>());
    assert(echo.at("result").get<bool>());
    assert(echo.at("message") == "hello world");

    const auto typed = Invoke(ExampleCommand, {
        { "path", "/example types" },
        { "arguments", {
            { "text", "sample" }, { "count", 3 }, { "ratio", 1.25 }, { "enabled", true },
            { "mode", "fast" }, { "tags", { "alpha", "beta" } }, { "path", "C:/temp" },
            { "module", "Clock" }, { "target", "/about" }, { "payload", { { "source", "test" } } }
        } }
    });
    assert(typed.at("ok").get<bool>());
    const auto parsed = json::parse(typed.at("message").get<std::string>());
    assert(parsed.at("count") == 3);
    assert(parsed.at("ratio") == 1.25);
    assert(parsed.at("enabled").get<bool>());
    assert(parsed.at("mode") == "fast");
    assert(parsed.at("tags").size() == 2);
    assert(parsed.at("payload").at("source") == "test");

    const auto missing = Invoke(ExampleCommand, { { "path", "/missing" } });
    assert(!missing.at("ok").get<bool>());
}

void TestHooks() {
    const auto replace = Invoke(ExampleCommandHook, {
        { "phase", "before" }, { "path", "/version" }, { "command", "/version --example-replace" }
    });
    assert(replace.at("action") == "replace");
    assert(replace.at("command") == "/version");

    const auto before = Invoke(ExampleCommandHook, {
        { "phase", "before" }, { "path", "/version" }, { "command", "/version --example-override" }
    });
    assert(before.at("action") == "override");
    assert(before.at("result").get<bool>());

    const auto after = Invoke(ExampleCommandHook, {
        { "phase", "after" }, { "path", "/version" }, { "command", "/version --example-after-override" },
        { "result", true }
    });
    assert(after.at("action") == "override");
    assert(!after.at("result").get<bool>());
    assert(after.at("level") == "warning");

    const auto continued = Invoke(ExampleCommandHook, {
        { "phase", "before" }, { "path", "/version" }, { "command", "/version" }
    });
    assert(continued.at("action") == "continue");
}

void TestSmallResponseBuffer() {
    const auto input = json({ { "path", "/example types" }, { "arguments", { { "text", "small-buffer" } } } }).dump();
    char buffer[1]{};
    uint32_t required = 0;
    assert(ExampleCommand(input.c_str(), buffer, sizeof(buffer), &required) == JC_EXTENSION_BUFFER_TOO_SMALL);
    assert(required > 1);
}
}

int main() {
    TestCommands();
    TestHooks();
    TestSmallResponseBuffer();
    std::cout << "example command contract passed\n";
    return 0;
}
