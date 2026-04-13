#include "opencat/speech_planner.h"

#include <array>

namespace {

bool IsUtf8ContinuationByte(unsigned char byte) {
    return (byte & 0xC0U) == 0x80U;
}

size_t Utf8SafeTrimIndex(std::string_view text, size_t raw_limit) {
    if (raw_limit >= text.size()) {
        return text.size();
    }

    size_t i = raw_limit;
    while (i > 0 && IsUtf8ContinuationByte(static_cast<unsigned char>(text[i - 1]))) {
        --i;
    }
    if (i == 0) {
        return 0;
    }

    const size_t sequence_start = i - 1;
    const unsigned char lead = static_cast<unsigned char>(text[sequence_start]);
    size_t expected_len = 1;
    if ((lead & 0x80U) == 0x00U) {
        expected_len = 1;
    } else if ((lead & 0xE0U) == 0xC0U) {
        expected_len = 2;
    } else if ((lead & 0xF0U) == 0xE0U) {
        expected_len = 3;
    } else if ((lead & 0xF8U) == 0xF0U) {
        expected_len = 4;
    } else {
        return sequence_start;
    }

    const size_t available_len = raw_limit - sequence_start;
    if (available_len < expected_len) {
        return sequence_start;
    }
    return raw_limit;
}

}  // namespace

bool IsDetailSpeechRequested(std::string_view user_text) {
    static constexpr std::array<std::string_view, 6> kDetailTriggers = {
        "详细说", "详细讲", "展开说", "说详细点", "具体一点", "细说"};

    for (std::string_view trigger : kDetailTriggers) {
        if (user_text.find(trigger) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

SpeechPlan PlanOpenCatSpeech(std::string_view assistant_text, bool force_full, size_t short_limit) {
    SpeechPlan plan;
    if (assistant_text.empty()) {
        plan.text = "";
        plan.full_mode = false;
        return plan;
    }

    if (force_full || assistant_text.size() <= short_limit) {
        plan.text.assign(assistant_text);
        plan.full_mode = true;
        return plan;
    }

    const size_t safe_len = Utf8SafeTrimIndex(assistant_text, short_limit);
    plan.text.assign(assistant_text.substr(0, safe_len));
    plan.full_mode = false;
    return plan;
}
