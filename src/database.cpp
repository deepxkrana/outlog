#include "../include/outlog/database.hpp"
#include <iostream>

namespace outlog {
namespace db {

Database::Database(const std::string& path) : path_(path) {}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::initialize() {
    int rc = sqlite3_open(path_.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS executions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            command TEXT NOT NULL,
            stdout TEXT,
            stderr TEXT,
            working_directory TEXT NOT NULL,
            started_at TEXT NOT NULL,
            finished_at TEXT NOT NULL,
            duration_ms INTEGER NOT NULL,
            exit_code INTEGER NOT NULL,
            shell TEXT NOT NULL DEFAULT 'bash',
            output_recorded INTEGER NOT NULL DEFAULT 1,
            output_truncated INTEGER NOT NULL DEFAULT 0,
            recording_status TEXT NOT NULL DEFAULT 'complete'
        );
    )";

    char* error_msg = nullptr;
    rc = sqlite3_exec(db_, schema, nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error during initialization: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }

    return true;
}

bool Database::insert_record(const ExecutionRecord& record) {
    const char* sql = "INSERT INTO executions (command, stdout, stderr, working_directory, "
                      "started_at, finished_at, duration_ms, exit_code, shell, "
                      "output_recorded, output_truncated, recording_status) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, record.command.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, record.stdout_text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record.stderr_text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, record.working_directory.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, record.started_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, record.finished_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, record.duration_ms);
    sqlite3_bind_int(stmt, 8, record.exit_code);
    sqlite3_bind_text(stmt, 9, record.shell.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, record.output_recorded ? 1 : 0);
    sqlite3_bind_int(stmt, 11, record.output_truncated ? 1 : 0);
    sqlite3_bind_text(stmt, 12, record.recording_status.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!success) {
        std::cerr << "Failed to insert record: " << sqlite3_errmsg(db_) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

void Database::print_all(bool details) {
    const char* sql = details ? "SELECT id, command, started_at, exit_code, duration_ms, stdout, stderr FROM executions ORDER BY id ASC;" 
                              : "SELECT id, command, started_at, exit_code, duration_ms FROM executions ORDER BY id ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    std::cout << "All recorded commands:\n";
    std::cout << "ID\tExit\tTime\t\t\tDuration(ms)\tCommand\n";
    std::cout << "--------------------------------------------------------------------------\n";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* cmd = sqlite3_column_text(stmt, 1);
        const unsigned char* started = sqlite3_column_text(stmt, 2);
        int exit_code = sqlite3_column_int(stmt, 3);
        int duration = sqlite3_column_int(stmt, 4);

        std::cout << id << "\t" << exit_code << "\t" << (started ? (const char*)started : "") 
                  << "\t" << duration << "\t\t" << (cmd ? (const char*)cmd : "") << "\n";
                  
        if (details) {
            const unsigned char* out = sqlite3_column_text(stmt, 5);
            const unsigned char* err = sqlite3_column_text(stmt, 6);
            if (out && out[0] != '\0') {
                std::cout << "--- STDOUT ---\n" << (const char*)out;
                // Add a newline if stdout doesn't end with one
                std::string out_str((const char*)out);
                if (!out_str.empty() && out_str.back() != '\n') std::cout << "\n";
            }
            if (err && err[0] != '\0') {
                std::cout << "--- STDERR ---\n" << (const char*)err;
                std::string err_str((const char*)err);
                if (!err_str.empty() && err_str.back() != '\n') std::cout << "\n";
            }
            std::cout << "--------------------------------------------------------------------------\n";
        }
    }

    sqlite3_finalize(stmt);
}

void Database::print_recent(int count) {
    const char* sql = "SELECT id, command, started_at, exit_code, duration_ms FROM executions ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, count);

    std::cout << "Recent commands:\n";
    std::cout << "ID\tExit\tTime\tDuration(ms)\tCommand\n";
    std::cout << "---------------------------------------------------------\n";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* cmd = sqlite3_column_text(stmt, 1);
        const unsigned char* started = sqlite3_column_text(stmt, 2);
        int exit_code = sqlite3_column_int(stmt, 3);
        int duration = sqlite3_column_int(stmt, 4);

        std::cout << id << "\t" << exit_code << "\t" << (started ? (const char*)started : "") 
                  << "\t" << duration << "\t\t" << (cmd ? (const char*)cmd : "") << "\n";
    }

    sqlite3_finalize(stmt);
}

void Database::print_record(int id) {
    const char* sql = "SELECT * FROM executions WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::cout << "--- Record " << id << " ---\n";
        for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
            const char* col_name = sqlite3_column_name(stmt, i);
            const unsigned char* col_val = sqlite3_column_text(stmt, i);
            std::cout << col_name << ": " << (col_val ? (const char*)col_val : "NULL") << "\n";
        }
    } else {
        std::cout << "Record " << id << " not found.\n";
    }

    sqlite3_finalize(stmt);
}

void Database::search(const std::string& query) {
    const char* sql = "SELECT id, command, started_at, exit_code, duration_ms FROM executions WHERE command LIKE ? OR stdout LIKE ? OR stderr LIKE ? ORDER BY id DESC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    std::string like_query = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, like_query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, like_query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, like_query.c_str(), -1, SQLITE_TRANSIENT);

    std::cout << "Search results for '" << query << "':\n";
    std::cout << "ID\tExit\tTime\tDuration(ms)\tCommand\n";
    std::cout << "---------------------------------------------------------\n";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* cmd = sqlite3_column_text(stmt, 1);
        const unsigned char* started = sqlite3_column_text(stmt, 2);
        int exit_code = sqlite3_column_int(stmt, 3);
        int duration = sqlite3_column_int(stmt, 4);

        std::cout << id << "\t" << exit_code << "\t" << (started ? (const char*)started : "") 
                  << "\t" << duration << "\t\t" << (cmd ? (const char*)cmd : "") << "\n";
    }

    sqlite3_finalize(stmt);
}

void Database::delete_record(int id) {
    const char* sql = "DELETE FROM executions WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare delete statement: " << sqlite3_errmsg(db_) << std::endl;
        return;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to delete record: " << sqlite3_errmsg(db_) << std::endl;
    } else {
        if (sqlite3_changes(db_) > 0) {
            std::cout << "Deleted record " << id << ".\n";
        } else {
            std::cout << "Record " << id << " not found.\n";
        }
    }

    sqlite3_finalize(stmt);
}

void Database::clear_all() {
    const char* sql = "DELETE FROM executions;";
    char* error_msg = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &error_msg) != SQLITE_OK) {
        std::cerr << "Failed to clear records: " << error_msg << std::endl;
        sqlite3_free(error_msg);
    } else {
        std::cout << "Cleared all records.\n";
    }
}

} // namespace db
} // namespace outlog
