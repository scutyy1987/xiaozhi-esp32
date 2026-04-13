#ifndef OPEN_CAT_BRIDGE_H_
#define OPEN_CAT_BRIDGE_H_

#include <functional>
#include <string_view>

#include "opencat/bt_command_gateway.h"

class OpenCatBridge {
public:
    OpenCatBridge();
    ~OpenCatBridge();

    void SetGatewaySender(std::function<bool(std::string_view)> sender);
    void OnUserText(std::string_view user_text);
    void OnAssistantSentence(std::string_view assistant_text, bool force_full = false);

    BtCommandGateway::Stats GetStats() const;

private:
    BtCommandGateway gateway_;
};

#endif  // OPEN_CAT_BRIDGE_H_
