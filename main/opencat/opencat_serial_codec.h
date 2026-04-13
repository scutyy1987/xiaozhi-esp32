#ifndef OPEN_CAT_SERIAL_CODEC_H_
#define OPEN_CAT_SERIAL_CODEC_H_

#include <string>
#include <string_view>

std::string EncodeOpenCatSkillCommand(std::string_view skill, bool append_newline = true);
std::string EncodeOpenCatRenameCommand(std::string_view name, bool append_newline = true);

#endif  // OPEN_CAT_SERIAL_CODEC_H_
