#include <iostream>
#include "../include/outlog/cli.hpp"
#include "../include/outlog/database.hpp"

int main(int argc, char* argv[]) {
    // Initialize database in a default location for testing
    // e.g. ~/.local/share/outlog/outlog.db or just local outlog.db for now
    std::string db_path = "outlog.db";
    outlog::db::Database db(db_path);
    
    if (!db.initialize()) {
        std::cerr << "Failed to initialize database." << std::endl;
        return 1;
    }

    return outlog::cli::parse_and_run(argc, argv, db);
}
