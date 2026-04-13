#ifndef OPEN_CAT_SPEECH_PLANNER_H_
#define OPEN_CAT_SPEECH_PLANNER_H_

#include <cstddef>
#include <string>
#include <string_view>

struct SpeechPlan {
    std::string text;
    bool full_mode;
};

bool IsDetailSpeechRequested(std::string_view user_text);
SpeechPlan PlanOpenCatSpeech(std::string_view assistant_text, bool force_full = false, size_t short_limit = 40);

#endif  // OPEN_CAT_SPEECH_PLANNER_H_
