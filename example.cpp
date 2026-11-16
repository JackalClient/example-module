#include "jackal_extension.hpp"
#include <cstring>
#include <string>

namespace {
using nlohmann::json;
jackal::Host host;
bool enabled = false, previousState = false, lastState = false, managed = false, restore = false;
std::string watched;
json lastConfig;

json WatchedConfig(const std::string& module) {
    auto config = host.config(module);
    for (const char* key : { "Enabled", "hidden", "Groups", "VisibleWhen", "VisibleWhenBatch" }) config.erase(key);
    return config;
}

std::string OptionText(const json& config, const char* key, const char* fallback) {
    const auto found = config.find(key);
    if (found == config.end()) return fallback;
    if (found->is_string()) return found->get<std::string>();
    if (found->is_object() && found->contains("TextArea")) return found->at("TextArea").get<std::string>();
    if (found->is_object() && found->contains("Selected")) return found->at("Selected").get<std::string>();
    return fallback;
}

int CommandResponse(const json& response, char* buffer, uint32_t capacity, uint32_t* required) {
    return jackal::WriteResponse(response, buffer, capacity, required);
}
}

extern "C" uint32_t __cdecl JcExtensionAbiVersion() { return JC_EXTENSION_ABI_VERSION; }
extern "C" int __cdecl JcExtensionInitialize(const JcExtensionHostV1* environment) { return host.initialize(environment); }
extern "C" int __cdecl ExampleCommand(const char* requestUtf8, char* responseUtf8, uint32_t capacity, uint32_t* required) {
    try {
        const auto request = json::parse(requestUtf8);
        const auto path = request.at("path").get<std::string>();
        if (path == "/example" || path == "/ex") {
            return CommandResponse(jackal::CommandResult(true,
                "Usage: /example echo <text> | /example types ...", "info"), responseUtf8, capacity, required);
        }
        if (path == "/example echo" || path == "/ex echo") {
            return CommandResponse(jackal::CommandResult(true,
                request.at("arguments").at("text").get<std::string>(), "success"), responseUtf8, capacity, required);
        }
        if (path == "/example types") {
            return CommandResponse(jackal::CommandResult(true,
                request.at("arguments").dump(), "success"), responseUtf8, capacity, required);
        }
        return CommandResponse(jackal::CommandResult(false, "Unknown example subcommand.", "error"), responseUtf8, capacity, required);
    }
    catch (const std::exception& error) {
        return CommandResponse({ { "ok", false }, { "error", error.what() } }, responseUtf8, capacity, required);
    }
    catch (...) {
        return CommandResponse({ { "ok", false }, { "error", "Example command failed." } }, responseUtf8, capacity, required);
    }
}
extern "C" int __cdecl ExampleCommandHook(const char* requestUtf8, char* responseUtf8, uint32_t capacity, uint32_t* required) {
    try {
        const auto request = json::parse(requestUtf8);
        const auto command = request.at("command").get<std::string>();
        const auto phase = request.at("phase").get<std::string>();
        if (phase == "before" && command.find("--example-replace") != std::string::npos)
            return CommandResponse(jackal::HookReplace("/version"), responseUtf8, capacity, required);
        if (phase == "before" && command.find("--example-override") != std::string::npos)
            return CommandResponse(jackal::HookOverride(true, "Before hook override."), responseUtf8, capacity, required);
        if (phase == "after" && command.find("--example-after-override") != std::string::npos)
            return CommandResponse(jackal::HookOverride(false, "After hook override.", "warning"), responseUtf8, capacity, required);
        return CommandResponse(jackal::HookContinue(), responseUtf8, capacity, required);
    }
    catch (const std::exception& error) {
        return CommandResponse({ { "ok", false }, { "error", error.what() } }, responseUtf8, capacity, required);
    }
    catch (...) {
        return CommandResponse({ { "ok", false }, { "error", "Example command hook failed." } }, responseUtf8, capacity, required);
    }
}
extern "C" int __cdecl ExampleEnable(const char* configUtf8) {
    if (enabled) return 1;
    try {
        const auto config = json::parse(configUtf8);
        const auto target = config.value("Watched Module", "Clock");
        if (target == "ExampleExtension") return 0;
        const bool state = host.moduleEnabled(target);
        const auto targetConfig = WatchedConfig(target);
        const int count = config.value("Activation Count", 0);
        host.post({ { "method", "config.set" }, { "module", "ExampleExtension" }, { "key", "Activation Count" }, { "value", count < 1000000 ? count + 1 : 1 } });
        watched = target; previousState = lastState = state;
        lastConfig = targetConfig;
        managed = config.value("Control Watched Module", false); restore = config.value("Restore On Disable", true);
        if (managed && !state) host.setModule(watched, true);
        host.notify(OptionText(config, "Message", "ExampleExtension enabled.") + "\n" + watched + ": " + (state ? "On" : "Off"),
            OptionText(config, "Notification Level", "info"), "ExampleExtension");
        enabled = true;
        return 1;
    } catch (...) { return 0; }
}
extern "C" int __cdecl ExampleDisable() {
    if (!enabled) return 1;
    try {
        if (managed && restore && host.moduleEnabled(watched) != previousState) host.setModule(watched, previousState);
        host.notify("ExampleExtension disabled.", "info", "ExampleExtension");
        enabled = false;
        return 1;
    } catch (...) { return 0; }
}
extern "C" int __cdecl ExampleIsEnabled() { return enabled ? 1 : 0; }
extern "C" int __cdecl ExampleUpdate(const char* configUtf8) {
    if (!enabled) return 1;
    try {
        const auto config = json::parse(configUtf8);
        const bool state = host.moduleEnabled(watched);
        const auto targetConfig = WatchedConfig(watched);
        if (state != lastState) {
            if (config.value("Notify Changes", true))
                host.notify(watched + (state ? " enabled" : " disabled"), OptionText(config, "Notification Level", "info"), "ExampleExtension");
            if (state && config.value("Run Command On Enable", false)) host.command(config.value("Enable Command", "/about"));
            if (!state && config.value("Run Command On Disable", false)) host.command(config.value("Disable Command", "/about"));
        }
        if (targetConfig != lastConfig && config.value("Notify Changes", true)) {
            std::string changed;
            for (const auto& option : targetConfig.items())
                if (!lastConfig.contains(option.key()) || lastConfig.at(option.key()) != option.value()) {
                    if (changed.size() > 1000) break;
                    if (!changed.empty()) changed += ", ";
                    changed += option.key();
                }
            for (const auto& option : lastConfig.items())
                if (!targetConfig.contains(option.key()) && changed.size() <= 1000) {
                    if (!changed.empty()) changed += ", ";
                    changed += option.key();
                }
            host.notify(watched + " configuration changed: " + changed, OptionText(config, "Notification Level", "info"), "ExampleExtension");
        }
        lastState = state;
        lastConfig = targetConfig;
        return 1;
    } catch (...) { return 0; }
}
