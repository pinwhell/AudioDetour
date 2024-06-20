#pragma once

#include <cstdint>
#include <optional>

std::optional<size_t> AskChooseOption(const char** optArr, size_t optArrCount);

#define ARR_SIZE(x) sizeof((x)) / sizeof((x)[0])