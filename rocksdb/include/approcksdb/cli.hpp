#pragma once

#include "approcksdb/types.hpp"

namespace approcksdb {

const char* FeatureName(unsigned long feature);
int ParseArgs(int argc, char* argv[], AppConfig* cfg);

} /* namespace approcksdb */