#include "opencat/rule_intent_parser.h"

#include <array>
#include <utility>

#include "opencat/action_whitelist.h"

namespace {

using RuleEntry = std::pair<std::string_view, std::string_view>;

constexpr std::array<RuleEntry, 24> kRuleEntries = {{
    {"往前走", "wkF"}, {"向前", "wkF"}, {"往前", "wkF"}, {"前进", "wkF"},
    {"往后走", "bkF"}, {"向后", "bkF"}, {"往后", "bkF"}, {"后退", "bkF"},
    {"向左", "wkL"},   {"往左", "wkL"}, {"左转", "wkL"},
    {"向右", "wkR"},   {"往右", "wkR"}, {"右转", "wkR"},
    {"坐下", "sit"},
    {"站起", "str"},   {"站立", "str"},
    {"趴下", "rest"},  {"趴着", "rest"},
    // Use "up" as stop fallback because it is in current whitelist.
    {"不要动", "up"},  {"别动", "up"}, {"停止", "up"},
}};

}  // namespace

RuleParseResult ParseOpenCatSkillByRules(std::string_view text) {
    for (const auto& [keyword, skill] : kRuleEntries) {
        if (text.find(keyword) == std::string_view::npos) {
            continue;
        }
        if (!IsAllowedOpenCatSkill(skill)) {
            return {false, ""};
        }
        return {true, std::string(skill)};
    }
    return {false, ""};
}
