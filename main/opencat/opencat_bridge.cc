#include "opencat/opencat_bridge.h"

#include <utility>

#include <esp_log.h>

#include "opencat/opencat_serial_codec.h"
#include "opencat/rule_intent_parser.h"
#include "opencat/speech_planner.h"

namespace {

const char* const kTag = "OpenCatBridge";

}  // namespace

OpenCatBridge::OpenCatBridge() {
    gateway_.Start();
}

OpenCatBridge::~OpenCatBridge() {
    gateway_.Stop();
}

void OpenCatBridge::SetGatewaySender(std::function<bool(std::string_view)> sender) {
    gateway_.SetSender(std::move(sender));
}

void OpenCatBridge::OnUserText(std::string_view user_text) {
    if (user_text.empty()) {
        return;
    }

    const RuleParseResult result = ParseOpenCatSkillByRules(user_text);
    if (!result.hit) {
        // TODO: Add LLM fallback intent parser in next phase.
        return;
    }

    std::string payload = EncodeOpenCatSkillCommand(result.skill);
    if (payload.empty()) {
        return;
    }
    gateway_.Enqueue(std::move(payload));
}

void OpenCatBridge::OnAssistantSentence(std::string_view assistant_text, bool force_full) {
    if (assistant_text.empty()) {
        return;
    }

    SpeechPlan plan = PlanOpenCatSpeech(assistant_text, force_full);
    if (plan.text.empty()) {
        return;
    }

    // TODO: Replace "n" placeholder with real dog-side speech protocol.
    std::string payload = EncodeOpenCatRenameCommand(plan.text);
    if (payload.empty()) {
        return;
    }
    gateway_.Enqueue(std::move(payload));
}

BtCommandGateway::Stats OpenCatBridge::GetStats() const {
    return gateway_.GetStats();
}
