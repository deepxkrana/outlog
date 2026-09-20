#pragma once

#include "database.hpp"

namespace outlog {
namespace cli {

int parse_and_run(int argc, char* argv[], outlog::db::Database& db);

} // namespace cli
} // namespace outlog
