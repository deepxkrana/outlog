#pragma once

#include <chrono>
#include <string>
#include <functional>
#include <sys/types.h>

namespace outlog {
namespace pty {

struct CommandRecord {
    std::string command;
    int exit_code;
    std::string output;
    bool truncated{false};
    std::string working_directory;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point finished_at;
    int duration_ms{0};
};

// Absolute path of the running outlog binary, or "" if it can't be determined.
std::string executable_path();

// Quotes a string so bash treats it as a single literal word.
std::string shell_quote(const std::string& value);

class PtySession {
public:
    using OnCommandFinished = std::function<void(const CommandRecord&)>;

    PtySession() = default;
    ~PtySession();

    // Starts bash inside a pseudo-terminal. Returns true if successful.
    bool start();

    // Returns the file descriptor for the master end of the PTY
    int get_master_fd() const { return master_fd_; }
    
    // Connects STDIN/STDOUT to the PTY master
    void interact(OnCommandFinished callback = nullptr);

    // Wait for the bash process to exit and return its exit code
    int wait_for_exit();

private:
    int master_fd_{-1};
    pid_t child_pid_{-1};
};

} // namespace pty
} // namespace outlog
