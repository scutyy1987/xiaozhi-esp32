#include "opencat/opencat_serial_codec.h"

#include "opencat/action_whitelist.h"

namespace {

std::string BuildCommand(char prefix, std::string_view payload, bool append_newline) {
    std::string command;
    command.reserve(1 + payload.size() + (append_newline ? 1 : 0));
    command.push_back(prefix);
    command.append(payload);
    if (append_newline) {
        command.push_back('\n');
    }
    return command;
}

}  // namespace

std::string EncodeOpenCatSkillCommand(std::string_view skill, bool append_newline) {
    if (skill.empty() || !IsAllowedOpenCatSkill(skill)) {
        return "";
    }
    return BuildCommand('k', skill, append_newline);
}

std::string EncodeOpenCatRenameCommand(std::string_view name, bool append_newline) {
    if (name.empty()) {
        return "";
    }
    return BuildCommand('n', name, append_newline);
}
