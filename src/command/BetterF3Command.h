// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <vector>

namespace better_f3::command {

bool install();
void uninstall();

std::vector<std::string> run(std::string const& args, bool toChat = true);

}
