#ifndef OPEN_CAT_RULE_INTENT_PARSER_H_
#define OPEN_CAT_RULE_INTENT_PARSER_H_

#include <string>
#include <string_view>

struct RuleParseResult {
    bool hit;
    std::string skill;
};

RuleParseResult ParseOpenCatSkillByRules(std::string_view text);

#endif  // OPEN_CAT_RULE_INTENT_PARSER_H_
