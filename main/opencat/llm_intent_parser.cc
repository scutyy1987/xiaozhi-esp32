#include "opencat/llm_intent_parser.h"

#include "cJSON.h"
#include "opencat/action_whitelist.h"

namespace {

constexpr const char* kPromptPrefix =
    "You are an OpenCat skill intent classifier.\n"
    "You must choose `skill` only from the whitelist below.\n"
    "If you are not sure, set `skill` to null.\n"
    "Output JSON only, no explanation text.\n"
    "Allowed JSON schema exactly:\n"
    "{\"skill\": string|null, \"confidence\": number}\n"
    "Whitelist:\n";

constexpr const char* kPromptSuffix =
    "\nUser text:\n";

bool IsConfidenceInRange(float confidence) {
    return confidence >= 0.0f && confidence <= 1.0f;
}

}  // namespace

std::string BuildOpenCatIntentPrompt(std::string_view user_text) {
    std::string prompt;
    prompt.reserve(1024);
    prompt.append(kPromptPrefix);

    bool first = true;
    for (const auto& skill : GetOpenCatSkillWhitelist()) {
        if (!first) {
            prompt.append(", ");
        }
        prompt.append(skill.data(), skill.size());
        first = false;
    }

    prompt.append(kPromptSuffix);
    prompt.append(user_text.data(), user_text.size());
    return prompt;
}

LlmIntentParseResult ParseOpenCatIntentJson(std::string_view llm_json, float threshold) {
    LlmIntentParseResult result;
    result.skill = "";
    result.confidence = 0.0f;
    result.valid_json = false;
    result.accepted = false;

    cJSON* root = cJSON_ParseWithLength(llm_json.data(), llm_json.size());
    if (root == nullptr) {
        return result;
    }

    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return result;
    }

    cJSON* skill_item = cJSON_GetObjectItemCaseSensitive(root, "skill");
    cJSON* confidence_item = cJSON_GetObjectItemCaseSensitive(root, "confidence");
    if (skill_item == nullptr || confidence_item == nullptr || !cJSON_IsNumber(confidence_item)) {
        cJSON_Delete(root);
        return result;
    }

    const float confidence = static_cast<float>(confidence_item->valuedouble);
    if (!IsConfidenceInRange(confidence)) {
        cJSON_Delete(root);
        return result;
    }

    result.confidence = confidence;

    if (cJSON_IsNull(skill_item)) {
        result.valid_json = true;
        result.accepted = false;
        cJSON_Delete(root);
        return result;
    }

    if (!cJSON_IsString(skill_item) || skill_item->valuestring == nullptr) {
        cJSON_Delete(root);
        return result;
    }

    result.skill = skill_item->valuestring;
    result.valid_json = true;
    if (result.skill.empty()) {
        result.accepted = false;
        cJSON_Delete(root);
        return result;
    }

    if (!IsAllowedOpenCatSkill(result.skill)) {
        result.accepted = false;
        cJSON_Delete(root);
        return result;
    }

    if (result.confidence < threshold) {
        result.accepted = false;
        cJSON_Delete(root);
        return result;
    }

    result.accepted = true;
    cJSON_Delete(root);
    return result;
}
