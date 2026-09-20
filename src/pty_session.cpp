#include "../include/outlog/pty_session.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <fstream>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <util.h> // For forkpty on macOS

namespace outlog {
namespace pty {

static int g_master_fd = -1;

static void sigwinch_handler(int) {
    if (g_master_fd >= 0) {
        struct winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
            ioctl(g_master_fd, TIOCSWINSZ, &ws);
        }
    }
}

PtySession::~PtySession() {
    if (master_fd_ >= 0) {
        close(master_fd_);
    }
}

bool PtySession::start() {
    setenv("OUTLOG_ACTIVE", "1", 1);
    child_pid_ = forkpty(&master_fd_, nullptr, nullptr, nullptr);
    
    if (child_pid_ < 0) {
        std::cerr << "forkpty failed: " << strerror(errno) << std::endl;
        return false;
    } else if (child_pid_ == 0) {
        // Child process
        std::string rc_path = "/tmp/outlog_bashrc_" + std::to_string(getpid());
        std::ofstream rc_file(rc_path);
        rc_file << "if [ -f ~/.bashrc ]; then source ~/.bashrc; fi\n";
        rc_file << "if [ -f /usr/share/outlog/bash-integration.sh ]; then\n";
        rc_file << "    source /usr/share/outlog/bash-integration.sh\n";
        rc_file << "elif [ -f /Users/deepakrana/Desktop/outlog/scripts/bash-integration.sh ]; then\n";
        rc_file << "    source /Users/deepakrana/Desktop/outlog/scripts/bash-integration.sh\n";
        rc_file << "fi\n";
        rc_file << "rm -f " << rc_path << "\n"; // Self-cleanup
        rc_file.close();

        const char* args[] = {"bash", "--rcfile", rc_path.c_str(), nullptr};
        execvp(args[0], const_cast<char* const*>(args));
        
        std::cerr << "execvp bash failed: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    // Parent process
    return true;
}

void PtySession::interact(OnCommandFinished callback) {
    if (master_fd_ < 0) return;

    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    g_master_fd = master_fd_;
    sigwinch_handler(SIGWINCH);

    struct sigaction sa;
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, nullptr);

    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = master_fd_;
    pfds[1].events = POLLIN;

    char buf[4096];
    std::string output_buffer;
    bool recording = false;
    std::string current_cmd_output;
    bool truncated = false;
    const size_t MAX_OUTPUT_SIZE = 1024 * 1024; // 1 MB

    while (true) {
        if (poll(pfds, 2, -1) < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (pfds[0].revents & POLLIN) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n <= 0) break; // EOF on stdin
            
            // Write user input to the PTY
            write(master_fd_, buf, n);
        }

        if (pfds[1].revents & POLLIN) {
            ssize_t n = read(master_fd_, buf, sizeof(buf));
            if (n <= 0) break; // EOF from child process (bash exited)
            
            output_buffer.append(buf, n);
            
            while (!output_buffer.empty()) {
                size_t osc_start = output_buffer.find("\033]1337;Outlog;");
                if (osc_start == std::string::npos) {
                    // No OSC found, flush everything
                    write(STDOUT_FILENO, output_buffer.data(), output_buffer.size());
                    if (recording) {
                        if (current_cmd_output.size() + output_buffer.size() < MAX_OUTPUT_SIZE) {
                            current_cmd_output.append(output_buffer);
                        } else if (!truncated) {
                            size_t allowed = MAX_OUTPUT_SIZE > current_cmd_output.size() ? MAX_OUTPUT_SIZE - current_cmd_output.size() : 0;
                            current_cmd_output.append(output_buffer.substr(0, allowed));
                            current_cmd_output.append("\r\n...[output truncated by outlog]...\r\n");
                            truncated = true;
                        }
                    }
                    output_buffer.clear();
                    break;
                } else {
                    // Flush everything before the OSC
                    if (osc_start > 0) {
                        write(STDOUT_FILENO, output_buffer.data(), osc_start);
                        if (recording) {
                            if (current_cmd_output.size() + osc_start < MAX_OUTPUT_SIZE) {
                                current_cmd_output.append(output_buffer.substr(0, osc_start));
                            } else if (!truncated) {
                                size_t allowed = MAX_OUTPUT_SIZE > current_cmd_output.size() ? MAX_OUTPUT_SIZE - current_cmd_output.size() : 0;
                                current_cmd_output.append(output_buffer.substr(0, allowed));
                                current_cmd_output.append("\r\n...[output truncated by outlog]...\r\n");
                                truncated = true;
                            }
                        }
                    }
                    
                    size_t osc_end = output_buffer.find('\007', osc_start);
                    if (osc_end == std::string::npos) {
                        // Incomplete OSC, keep it in buffer for next read
                        output_buffer = output_buffer.substr(osc_start);
                        break;
                    }
                    
                    // Parse the OSC payload
                    std::string payload = output_buffer.substr(osc_start + 14, osc_end - (osc_start + 14));
                    if (payload == "Start") {
                        recording = true;
                        truncated = false;
                        current_cmd_output.clear();
                    } else if (payload.substr(0, 4) == "End;") {
                        recording = false;
                        
                        if (callback) {
                            CommandRecord rec;
                            rec.output = current_cmd_output;
                            rec.truncated = truncated;
                            
                            size_t first_semi = payload.find(';', 4);
                            if (first_semi != std::string::npos) {
                                rec.exit_code = std::stoi(payload.substr(4, first_semi - 4));
                                rec.command = payload.substr(first_semi + 1);
                            } else {
                                rec.exit_code = 0;
                            }
                            callback(rec);
                        }
                    }
                    
                    // Remove the processed OSC from buffer
                    output_buffer.erase(0, osc_end + 1);
                }
            }
        }
        
        if (pfds[1].revents & (POLLERR | POLLHUP)) {
            break; // Child PTY closed
        }
    }

    g_master_fd = -1;
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

int PtySession::wait_for_exit() {
    if (child_pid_ < 0) return -1;
    
    int status;
    waitpid(child_pid_, &status, 0);
    
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
}

} // namespace pty
} // namespace outlog
