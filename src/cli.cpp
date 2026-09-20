#include "../include/outlog/cli.hpp"
#include "../include/outlog/process_manager.hpp"
#include "../include/outlog/pty_session.hpp"
#if __has_include(<CLI/CLI.hpp>)
#include <CLI/CLI.hpp>
#else
#include "../build/_deps/cli11-src/include/CLI/CLI.hpp"
#endif
#include <iostream>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <cstdlib>

namespace outlog {
namespace cli {

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string get_working_dir() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        return std::string(cwd);
    }
    return "";
}

std::string get_config_dir() {
    const char* home = getenv("HOME");
    if (!home) return "";
    return std::string(home) + "/.config/outlog";
}

bool is_enabled() {
    std::string enabled_file = get_config_dir() + "/enabled";
    struct stat st;
    return stat(enabled_file.c_str(), &st) == 0;
}

void enable_outlog() {
    std::string config_dir = get_config_dir();
    // Create ~/.config if missing, then ~/.config/outlog
    std::string config_base = std::string(getenv("HOME")) + "/.config";
    mkdir(config_base.c_str(), 0755);
    mkdir(config_dir.c_str(), 0755);
    std::string enabled_file = config_dir + "/enabled";
    std::ofstream f(enabled_file);
    f << "1\n";
    f.close();
    
    std::string bashrc = std::string(getenv("HOME")) + "/.bashrc";
    std::ifstream in(bashrc);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    
    if (content.find("outlog shell") == std::string::npos) {
        std::ofstream out(bashrc, std::ios::app);
        out << "\n# Outlog integration\n";
        out << "if [ -z \"$OUTLOG_ACTIVE\" ] && [ -f ~/.config/outlog/enabled ]; then\n";
        out << "    export OUTLOG_ACTIVE=1\n";
        out << "    exec /Users/deepakrana/Desktop/outlog/build/outlog shell\n";
        out << "fi\n";
    }
    std::cout << "outlog is now ENABLED. It will start automatically in new terminals.\n";
}

void disable_outlog() {
    std::string enabled_file = get_config_dir() + "/enabled";
    unlink(enabled_file.c_str());
    std::cout << "outlog is now DISABLED.\n";
}

bool is_paused() {
    std::string paused_file = get_config_dir() + "/paused";
    struct stat st;
    return stat(paused_file.c_str(), &st) == 0;
}

void pause_outlog() {
    std::string config_dir = get_config_dir();
    std::string config_base = std::string(getenv("HOME")) + "/.config";
    mkdir(config_base.c_str(), 0755);
    mkdir(config_dir.c_str(), 0755);
    std::string paused_file = config_dir + "/paused";
    std::ofstream f(paused_file);
    f << "1\n";
    f.close();
    std::cout << "outlog recording is now PAUSED in all terminals.\n";
}

void resume_outlog() {
    std::string paused_file = get_config_dir() + "/paused";
    unlink(paused_file.c_str());
    std::cout << "outlog recording is now RESUMED.\n";
}

bool is_ignored_program(const std::string& command) {
    const char* ignored[] = {"nano", "vim", "vi", "ssh", "htop", "top", "less", "more"};
    for (const char* p : ignored) {
        if (command == p || command.find(std::string(p) + " ") == 0) {
            return true;
        }
    }
    return false;
}

int parse_and_run(int argc, char* argv[], outlog::db::Database& db) {
    CLI::App app{"outlog - A command logger"};

    int last_n = 0;
    auto opt_n = app.add_option("-n,--last", last_n, "Show last N records");

    bool details = false;
    app.add_flag("-d,--details", details, "Show details (used with -c)");

    int record_id = -1;
    auto opt_id = app.add_option("-c,--id", record_id, "Specify record ID");

    std::string search_query;
    app.add_option("--search", search_query, "Search commands and output");

    bool failed = false;
    app.add_flag("--failed", failed, "Show failed commands");

    auto cmd_enable = app.add_subcommand("enable", "Enable automatic recording");
    auto cmd_disable = app.add_subcommand("disable", "Disable automatic recording");
    auto cmd_status = app.add_subcommand("status", "Show status");
    auto cmd_pause = app.add_subcommand("pause", "Pause recording temporarily");
    auto cmd_resume = app.add_subcommand("resume", "Resume recording");

    int limit = 10;
    auto cmd_clear = app.add_subcommand("clear", "Clear all records");
    
    int delete_id = -1;
    auto cmd_delete = app.add_subcommand("delete", "Delete a record by ID");
    cmd_delete->add_option("id", delete_id, "ID to delete")->required();

    auto cmd_run = app.add_subcommand("run", "Execute a command and record it");
    std::vector<std::string> run_args;
    cmd_run->add_option("command", run_args, "Command to execute")->required();

    auto cmd_shell = app.add_subcommand("shell", "Start an interactive PTY shell session");

    app.require_subcommand(0, 1); // 0 or 1 subcommands allowed

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (*cmd_clear) {
        db.clear_all();
    } else if (*cmd_enable) {
        enable_outlog();
    } else if (*cmd_disable) {
        disable_outlog();
    } else if (*cmd_status) {
        if (is_enabled()) {
            std::cout << "Status: ENABLED\n";
        } else {
            std::cout << "Status: DISABLED\n";
        }
        if (is_paused()) {
            std::cout << "Recording: PAUSED\n";
        } else {
            std::cout << "Recording: ACTIVE\n";
        }
        if (getenv("OUTLOG_ACTIVE")) {
            std::cout << "Active in current terminal: YES\n";
        } else {
            std::cout << "Active in current terminal: NO\n";
        }
    } else if (*cmd_pause) {
        pause_outlog();
    } else if (*cmd_resume) {
        resume_outlog();
    } else if (*cmd_delete) {
        db.delete_record(delete_id);
    } else if (*cmd_shell) {
        if (getenv("OUTLOG_ACTIVE") != nullptr) {
            std::cerr << "outlog shell is already active in this terminal to prevent recursion." << std::endl;
            return 1;
        }

        outlog::pty::PtySession session;
        if (session.start()) {
            session.interact([&db](const outlog::pty::CommandRecord& rec) {
                if (is_paused()) {
                    return; // Skip recording entirely
                }
                
                outlog::db::ExecutionRecord r;
                r.command = rec.command;
                r.working_directory = get_working_dir(); 
                r.started_at = get_current_time();
                r.finished_at = get_current_time();
                r.duration_ms = 0; 
                r.exit_code = rec.exit_code;
                r.shell = "bash";
                
                if (is_ignored_program(rec.command)) {
                    r.stdout_text = "";
                    r.output_recorded = false;
                    r.output_truncated = false;
                    r.recording_status = "skipped";
                } else {
                    r.stdout_text = rec.output;
                    r.output_recorded = true;
                    r.output_truncated = rec.truncated;
                    r.recording_status = "complete";
                }
                r.stderr_text = "";
                
                if (!db.insert_record(r)) {
                    std::cerr << "\r\nFailed to record execution.\r\n";
                }
            });
            session.wait_for_exit();
        }
    } else if (*cmd_run) {
        std::string start_time = get_current_time();
        auto start_tick = std::chrono::steady_clock::now();
        
        outlog::process::ProcessManager pm;
        auto result = pm.execute(run_args);
        
        auto end_tick = std::chrono::steady_clock::now();
        std::string end_time = get_current_time();
        int duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_tick - start_tick).count();

        // Save execution details to database
        outlog::db::ExecutionRecord r;
        r.command = run_args.empty() ? "" : run_args[0]; 
        for(size_t i=1; i<run_args.size(); ++i) r.command += " " + run_args[i];
        
        r.stdout_text = result.stdout_data;
        r.stderr_text = result.stderr_data;
        r.working_directory = get_working_dir();
        r.started_at = start_time;
        r.finished_at = end_time;
        r.duration_ms = duration_ms;
        r.exit_code = result.exit_code;
        r.shell = "bash";
        r.output_recorded = true;
        r.output_truncated = false;
        r.recording_status = "complete";
        
        if (!db.insert_record(r)) {
            std::cerr << "Failed to record execution.\n";
        }
    } else if (app.count("--search") > 0) {
        db.search(search_query);
    } else if (opt_id->count() > 0) {
        db.print_record(record_id);
    } else if (opt_n->count() > 0) {
        db.print_recent(last_n);
    } else if (app.get_subcommands().empty()) {
        db.print_all(details);
    } else {
        std::cout << "Command parsed successfully but not implemented yet.\n";
    }

    return 0;
}

} // namespace cli
} // namespace outlog
