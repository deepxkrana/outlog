#pragma once

#include <string>

#include <sqlite3.h>

namespace outlog {
namespace db {

struct ExecutionRecord {
    int id;
    std::string command;
    std::string stdout_text;
    std::string stderr_text;
    std::string working_directory;
    std::string started_at;
    std::string finished_at;
    int duration_ms;
    int exit_code;
    std::string shell;
    bool output_recorded;
    bool output_truncated;
    std::string recording_status;
};

class Database {
public:
    Database(const std::string& path);
    ~Database();

    bool initialize();
    bool insert_record(const ExecutionRecord& record);
    void print_recent(int count);
    void print_record(int id);
    void search(const std::string& query);
    void delete_record(int id);
    void clear_all();

private:
    sqlite3* db_{nullptr};
    std::string path_;
};

} // namespace db
} // namespace outlog
