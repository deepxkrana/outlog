#include "../include/outlog/process_manager.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <poll.h>

namespace outlog {
namespace process {

std::vector<char*> ProcessManager::to_c_args(const std::vector<std::string>& command) {
    std::vector<char*> c_args;
    c_args.reserve(command.size() + 1);
    for (const auto& arg : command) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);
    return c_args;
}

ProcessResult ProcessManager::execute(const std::vector<std::string>& command) {
    ProcessResult result;
    result.exit_code = -1;

    if (command.empty()) {
        return result;
    }

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        std::cerr << "Failed to create pipes: " << strerror(errno) << std::endl;
        return result;
    }

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        return result;
    } else if (pid == 0) {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        auto c_args = to_c_args(command);
        execvp(c_args[0], c_args.data());
        
        std::cerr << "execvp failed: " << strerror(errno) << std::endl;
        exit(127);
    } else {
        // Parent process
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        struct pollfd pfds[2];
        pfds[0].fd = stdout_pipe[0];
        pfds[0].events = POLLIN;
        pfds[1].fd = stderr_pipe[0];
        pfds[1].events = POLLIN;

        char buffer[4096];
        bool stdout_open = true;
        bool stderr_open = true;

        while (stdout_open || stderr_open) {
            int ret = poll(pfds, 2, -1);
            if (ret < 0) {
                if (errno == EINTR) continue;
                break;
            }

            if (stdout_open && (pfds[0].revents & POLLIN)) {
                ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    result.stdout_data.append(buffer, bytes_read);
                    std::cout.write(buffer, bytes_read);
                    std::cout.flush();
                } else if (bytes_read == 0) {
                    stdout_open = false;
                }
            } else if (pfds[0].revents & (POLLHUP | POLLERR)) {
                stdout_open = false;
            }

            if (stderr_open && (pfds[1].revents & POLLIN)) {
                ssize_t bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    result.stderr_data.append(buffer, bytes_read);
                    std::cerr.write(buffer, bytes_read);
                    std::cerr.flush();
                } else if (bytes_read == 0) {
                    stderr_open = false;
                }
            } else if (pfds[1].revents & (POLLHUP | POLLERR)) {
                stderr_open = false;
            }
        }

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.exit_code = 128 + WTERMSIG(status);
        }
        
        return result;
    }
}

} // namespace process
} // namespace outlog
