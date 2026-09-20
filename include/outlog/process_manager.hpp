#pragma once

#include <string>
#include <vector>

namespace outlog {
namespace process {

struct ProcessResult {
    int exit_code;
    std::string stdout_data;
    std::string stderr_data;
};

class ProcessManager {
public:
    ProcessManager() = default;
    ~ProcessManager() = default;

    // Executes a command and waits for it to finish.
    ProcessResult execute(const std::vector<std::string>& command);

private:
    // Helper to convert std::vector<std::string> to char* array for execvp
    std::vector<char*> to_c_args(const std::vector<std::string>& command);
};

} // namespace process
} // namespace outlog
