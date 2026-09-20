#undef NDEBUG
#include <cassert>
#include <iostream>
#include "../include/outlog/database.hpp"
#include <unistd.h>

int main() {
    std::cout << "Running Database Tests..." << std::endl;
    outlog::db::Database db(":memory:");
    assert(db.initialize() == true);
    
    outlog::db::ExecutionRecord r;
    r.command = "echo hello";
    r.stdout_text = "hello\n";
    r.stderr_text = "";
    r.working_directory = "/tmp";
    r.started_at = "2026-09-20 20:00:00";
    r.finished_at = "2026-09-20 20:00:01";
    r.duration_ms = 1000;
    r.exit_code = 0;
    r.shell = "bash";
    r.output_recorded = true;
    r.output_truncated = false;
    r.recording_status = "complete";
    
    assert(db.insert_record(r) == true);
    
    std::cout << "All Database Tests Passed!" << std::endl;
    return 0;
}
