#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include "../include/outlog/cli.hpp"
#include "../include/outlog/database.hpp"

namespace fs = std::filesystem;

// $XDG_DATA_HOME/outlog/outlog.db, defaulting to ~/.local/share/outlog/outlog.db
static std::string default_db_path() {
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && xdg[0] != '\0') {
        return (fs::path(xdg) / "outlog" / "outlog.db").string();
    }

    const char* home = getenv("HOME");
    if (!home || home[0] == '\0') {
        const passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : nullptr;
    }
    if (!home) {
        return "";
    }
    return (fs::path(home) / ".local" / "share" / "outlog" / "outlog.db").string();
}

// The database holds terminal output, so only its owner may read it.
static bool prepare_private_db(const std::string& db_path) {
    const fs::path dir = fs::path(db_path).parent_path();
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        std::cerr << "Can't create " << dir.string() << ": " << ec.message() << std::endl;
        return false;
    }
    fs::permissions(dir, fs::perms::owner_all, ec);
    if (ec) {
        std::cerr << "Can't secure " << dir.string() << ": " << ec.message() << std::endl;
        return false;
    }

    int fd = open(db_path.c_str(), O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        std::cerr << "Can't open " << db_path << std::endl;
        return false;
    }
    bool secured = fchmod(fd, 0600) == 0;
    close(fd);
    if (!secured) {
        std::cerr << "Can't secure " << db_path << std::endl;
    }
    return secured;
}

int main(int argc, char* argv[]) {
    std::string db_path = default_db_path();
    if (db_path.empty()) {
        std::cerr << "Can't determine the home directory for the database." << std::endl;
        return 1;
    }
    if (!prepare_private_db(db_path)) {
        return 1;
    }

    outlog::db::Database db(db_path);

    if (!db.initialize()) {
        std::cerr << "Failed to initialize database." << std::endl;
        return 1;
    }

    return outlog::cli::parse_and_run(argc, argv, db);
}
