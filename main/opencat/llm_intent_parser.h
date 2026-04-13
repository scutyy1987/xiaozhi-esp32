#ifndef OPEN_CAT_LLM_INTENT_PARSER_H_
#define OPEN_CAT_LLM_INTENT_PARSER_H_

#include <string>
#include <string_view>

struct LlmIntentParseResult {
    std::string skill;
    float confidence;
    bool valid_json;
    bool accepted;
};

std::string BuildOpenCatIntentPrompt(std::string_view user_text);
LlmIntentParseResult ParseOpenCatIntentJson(std::string_view llm_json, float threshold = 0.7f);

#endif  // OPEN_CAT_LLM_INTENT_PARSER_H_
