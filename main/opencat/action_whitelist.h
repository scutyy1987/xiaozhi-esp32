#ifndef OPEN_CAT_ACTION_WHITELIST_H_
#define OPEN_CAT_ACTION_WHITELIST_H_

#include <string_view>
#include <unordered_set>

const std::unordered_set<std::string_view>& GetOpenCatSkillWhitelist();
bool IsAllowedOpenCatSkill(std::string_view skill);

#endif  // OPEN_CAT_ACTION_WHITELIST_H_
