// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace better_f3 {

class Lang {
public:
    static Lang& get();

    void setDir(std::filesystem::path dir);
    void refresh();
    void force(std::string code);

    [[nodiscard]] std::string get(std::string const& key) const;
    [[nodiscard]] std::string const& code() const { return mCode; }

private:
    void load(std::string const& code);

    std::filesystem::path                        mDir;
    std::string                                  mCode;
    std::string                                  mGameCode;
    std::string                                  mForced;
    std::unordered_map<std::string, std::string> mCurrent;
    std::unordered_map<std::string, std::string> mFallback;
};

}
