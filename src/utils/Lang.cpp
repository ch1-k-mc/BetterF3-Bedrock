// SPDX-License-Identifier: MIT
#include "utils/Lang.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"

#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/options/IOptionRegistry.h"

namespace better_f3 {

namespace {

std::unordered_map<std::string, std::string> readLang(std::filesystem::path const& file) {
    std::unordered_map<std::string, std::string> out;
    std::ifstream                                in(file, std::ios::binary);
    if (!in) return out;
    std::stringstream ss;
    ss << in.rdbuf();
    auto j = nlohmann::json::parse(ss.str(), nullptr, false);
    if (!j.is_object()) {
        ll::mod::NativeMod::current()->getLogger().warn("BetterF3: cannot read the language file: {}", file.filename().string());
        return out;
    }
    for (auto& [k, v] : j.items()) {
        if (v.is_string()) out[k] = v.get<std::string>();
    }
    return out;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}

Lang& Lang::get() {
    static Lang instance;
    return instance;
}

void overlay(std::unordered_map<std::string, std::string>& base, std::filesystem::path const& file) {
    for (auto& [k, v] : readLang(file)) base[k] = v;
}

void Lang::setDir(std::filesystem::path dir) {
    mDir      = std::move(dir);
    mFallback = readLang(mDir / "en_us.json");
    overlay(mFallback, mDir / "bedrock" / "en_us.json");
    load("en_us");
}

void Lang::load(std::string const& code) {
    if (code == mCode) return;
    auto file = mDir / (code + ".json");
    mCode     = std::filesystem::exists(file) ? code : "en_us";
    mCurrent  = mCode == "en_us" ? mFallback : readLang(mDir / (mCode + ".json"));
    if (mCode != "en_us") overlay(mCurrent, mDir / "bedrock" / (mCode + ".json"));
    ll::mod::NativeMod::current()->getLogger().info("BetterF3: language {} ({} keys)", mCode, mCurrent.size());
}

void Lang::force(std::string code) {
    mForced = lower(std::move(code));
    mGameCode.clear();
    if (!mForced.empty()) load(mForced);
}

void Lang::refresh() {
    if (!mForced.empty()) return;
    auto ci = ll::service::getClientInstance();
    if (!ci) return;
    std::string game = ci->getOptions().getLanguage();
    if (game == mGameCode) return;
    mGameCode = game;
    load(lower(game));
}

std::string Lang::get(std::string const& key) const {
    if (auto it = mCurrent.find(key); it != mCurrent.end()) return it->second;
    if (auto it = mFallback.find(key); it != mFallback.end()) return it->second;
    return key;
}

}
