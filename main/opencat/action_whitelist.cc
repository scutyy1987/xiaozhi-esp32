#include "opencat/action_whitelist.h"

const std::unordered_set<std::string_view>& GetOpenCatSkillWhitelist() {
    static const std::unordered_set<std::string_view> kWhitelist = {
        "bdF",    "bk",      "bkArmF", "bkArmL", "bkF",    "bkL",    "carpetF",
        "carpetL","crArmF",  "crArmL", "crF",    "crL",    "gpF",    "gpL",
        "hlw",    "jpF",     "lftF",   "lftL",   "phF",    "phL",    "trArmF",
        "trArmL", "trF",     "trL",    "vtArmF", "vtF",    "vtL",    "wkArmF",
        "wkArmL", "wkF",     "wkL",    "balance","buttUp", "calib",  "dropped",
        "lifted", "lnd",     "rest",   "sit",    "str",    "up",     "zero",
        "ang",    "bf",      "bx",     "chr",    "ck",     "clap",   "cmh",
        "dg",     "dropRec", "ff",     "fiv",    "flip",   "flipD",  "flipF",
        "gdb",    "hds",     "hg",     "hi",     "hsk",    "hu",     "hunt",
        "jmp",    "kc",      "knock",  "launch", "lpov",   "lucky",  "mw",
        "nd",     "pd",      "pee",    "pick",   "pickD",  "pickF",  "pu",
        "pu1",    "put",     "putD",   "putF",   "rc",     "rl",     "scrh",
        "showOff","snf",     "tbl",    "toss",   "tossD",  "tossF",  "ts",
        "wh",     "zz",
    };
    return kWhitelist;
}

bool IsAllowedOpenCatSkill(std::string_view skill) {
    const auto& whitelist = GetOpenCatSkillWhitelist();
    return whitelist.find(skill) != whitelist.end();
}
