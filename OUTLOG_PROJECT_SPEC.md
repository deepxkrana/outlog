# Outlog

> A native Linux terminal command that automatically records shell commands, terminal output, timestamps, working directories, and exit codes for later inspection.

## 1. Project Overview

`outlog` is a terminal-only command designed to extend the concept of shell history.

Traditional shell history stores commands such as:

```text
terraform apply
pwd
docker inspect container_name
```

However, it usually does not preserve the output produced by those commands. This becomes a problem when:

- The terminal is closed.
- The computer is shut down.
- The output is needed later.
- Re-running the command could be dangerous.
- The command is not idempotent.
- The command created or changed infrastructure.
- The command output contained important information that was not copied elsewhere.

`outlog` solves this problem by automatically recording shell-level command executions and their outputs.

The user should not need to manually start a recording session.

After installation:

```bash
sudo apt install outlog
```

the user gives permission to enable automatic recording. After that, when the user opens a terminal, `outlog` starts automatically and works invisibly in the background of the shell session.

The user continues using the terminal normally:

```bash
ls
pwd
mkdir project
terraform plan
terraform validate
terraform apply
docker inspect container_name
```

Later, the user can inspect previous executions:

```bash
outlog -n 5
outlog -d -c 5
outlog --search terraform
outlog --failed
```

---

## 2. Project Goals

### Primary goals

`outlog` should:

1. Work as a native terminal command.
2. Initially support Linux and Bash.
3. Start automatically when an interactive Bash terminal opens.
4. Require no command such as `outlog --start`.
5. Record the command typed by the user.
6. Record command output.
7. Record standard output and standard error.
8. Record timestamps.
9. Record the command exit code.
10. Record the working directory.
11. Store records persistently across reboots.
12. Allow users to inspect previous commands without rerunning them.
13. Avoid recording raw keystrokes.
14. Avoid recording the contents typed into editors such as `nano` or `vim`.
15. Avoid requiring root privileges during normal operation.
16. Provide safe enable, disable, pause, resume, and uninstall behavior.

### Secondary goals

Future versions may support:

- Zsh
- Fish
- macOS
- Homebrew
- Fedora/RHEL packages
- Arch Linux packages
- Unix-like systems
- Output search
- Command tags
- Project-specific logs
- Output redaction
- Encrypted local storage
- Export to Markdown, JSON, or plain text
- Terminal replay

---

## 3. Non-Goals

The first version should not attempt to:

- Replace Bash.
- Modify Bash source code.
- Record every keyboard keystroke.
- Record text entered inside `nano`, `vim`, or other editors.
- Record passwords typed into hidden password prompts.
- Provide a web interface.
- Provide a GUI.
- Use a remote server.
- Synchronize logs to the cloud.
- Support every shell immediately.
- Support every Linux distribution immediately.
- Run permanently as root.
- Guarantee perfect output interpretation for every terminal application.

`outlog` is a terminal command, not a web application.

---

## 4. Target Platform for Version 1

The first release should have a narrow and well-defined scope:

```text
Operating system: Linux
Distribution target: Debian/Ubuntu
Shell: Bash
Language: C++
Standard: C++20
Database: SQLite
Package format: Debian .deb
Interface: Terminal only
```

The first version should be tested primarily on:

- Ubuntu LTS
- Debian stable
- Bash
- x86_64 Linux

Support for other platforms can be added after the Bash/Linux version is stable.

---

## 5. User Experience

### Installation

The user runs:

```bash
sudo apt install outlog
```

During installation, the package may ask:

```text
Enable automatic command and output recording for this user? [Y/n]
```

If the user chooses yes:

- The automatic Bash integration is enabled.
- Future interactive Bash sessions start through `outlog`.
- The user does not need to manually start recording.
- The user continues to see a normal shell prompt.

If the user chooses no:

- `outlog` is still installed.
- Automatic recording is disabled.
- The user can enable it later.

Example future commands:

```bash
outlog enable
outlog disable
outlog status
```

### Normal usage

The user opens a terminal normally:

```text
Terminal opens
        ↓
outlog starts automatically
        ↓
Bash starts inside outlog
        ↓
Normal shell prompt appears
```

The user runs commands normally:

```bash
pwd
ls -la
terraform plan
terraform apply
docker inspect my-container
```

The user should not notice a separate recording process.

### Viewing recent commands

```bash
outlog -n 5
```

Example output:

```text
Execution 41
Command: pwd
Output:
  /home/alice/projects/infrastructure

Execution 42
Command: terraform plan
Output:
  Plan: 2 to add, 0 to change, 0 to destroy.

Execution 43
Command: terraform apply
Output:
  Apply complete! Resources: 2 added, 0 changed, 0 destroyed.

Execution 44
Command: docker inspect my-container
Output:
  [
      {
          "Id": "..."
      }
  ]
```

### Viewing detailed information

```bash
outlog -d -c 43
```

Example:

```text
Execution ID: 43

Command:
terraform apply

Working directory:
/home/alice/projects/infrastructure

Started:
2026-09-20 14:32:11

Finished:
2026-09-20 14:34:03

Duration:
00:01:52

Exit code:
0

Output:
Apply complete! Resources: 2 added, 0 changed, 0 destroyed.
```

### Searching commands

```bash
outlog --search terraform
```

### Showing failed commands

```bash
outlog --failed
```

### Showing commands from a specific date

```bash
outlog --since yesterday
```

### Pausing and resuming

```bash
outlog pause
outlog resume
```

When paused, commands may still be recorded as metadata, but their output should not be stored.

Alternatively, paused mode may skip recording entirely. This behavior should be documented and configurable.

### Deleting records

```bash
outlog delete 43
```

### Clearing history

```bash
outlog clear
```

This command should require confirmation:

```text
This will permanently delete all outlog records. Continue? [y/N]
```

---

## 6. Example Use Case

The user runs the following commands:

```bash
pwd
ls
mkdir infrastructure
cd infrastructure
terraform init
terraform validate
terraform plan
terraform apply
```

The user then shuts down the computer.

The next day, the user wants to remember the output of `terraform apply`, but does not want to run it again because it may modify infrastructure.

The user runs:

```bash
outlog --search "terraform apply"
```

`outlog` returns:

```text
Execution 27
Command: terraform apply
Directory: /home/alice/infrastructure
Exit code: 0
Started: 2026-09-19 18:22:10
```

The user requests the complete record:

```bash
outlog -d -c 27
```

The original output is displayed without rerunning Terraform.

---

## 7. High-Level Architecture

The core architecture should place `outlog` between the terminal and the shell.

Normal shell architecture:

```text
Terminal emulator
        ↓
Bash
        ↓
Commands
```

Outlog architecture:

```text
Terminal emulator
        ↓
      outlog
        ↓
      Bash
        ↓
    Commands
```

The user should still see and interact with a normal Bash session.

Internally:

```text
Keyboard input
      ↓
Terminal
      ↓
outlog
      ↓
Bash
      ↓
Command

Command output
      ↓
Bash
      ↓
outlog
      ├── forwarded to terminal
      └── saved to storage
```

`outlog` acts as a transparent shell-session supervisor.

---

## 8. Why a PTY Is Required

A regular pipe is insufficient for a realistic terminal session.

Many programs behave differently when connected to a terminal versus a pipe.

Examples:

- `vim`
- `nano`
- `less`
- `top`
- `htop`
- `ssh`
- `sudo`
- interactive Python
- nested shells
- programs using colors
- programs using progress bars

Therefore, `outlog` should use a pseudo-terminal, commonly called a PTY.

The PTY design is:

```text
Real terminal
      ↕
outlog process
      ↕
PTY master
      ↕
PTY slave
      ↕
Bash
```

The Bash process should believe it is connected to a normal terminal.

The user should retain normal terminal behavior, including:

- command input
- arrow keys
- tab completion
- colors
- `Ctrl+C`
- `Ctrl+Z`
- `Ctrl+D`
- terminal resizing
- interactive programs

---

## 9. Core Components

### 9.1 CLI Layer

Responsible for parsing commands such as:

```bash
outlog
outlog -n 5
outlog -d -c 5
outlog --search terraform
outlog --failed
outlog enable
outlog disable
outlog pause
outlog resume
```

Possible implementation options:

- Manual argument parsing for the first version.
- CLI11 for a more complete argument parser.

### 9.2 PTY Session Manager

Responsible for:

- Creating the pseudo-terminal.
- Starting Bash.
- Connecting Bash to the PTY.
- Forwarding input.
- Forwarding output.
- Detecting EOF.
- Handling terminal size changes.
- Managing process groups.

Relevant Linux APIs may include:

```cpp
fork()
execve()
waitpid()
posix_openpt()
grantpt()
unlockpt()
ptsname()
ioctl()
setsid()
dup2()
poll()
select()
read()
write()
```

### 9.3 Process Manager

Responsible for:

- Starting Bash.
- Tracking child processes.
- Waiting for command completion.
- Capturing exit status.
- Handling signals.
- Cleaning up child processes.
- Avoiding zombie processes.

### 9.4 Command Tracker

Responsible for identifying:

- Command start.
- Command completion.
- Command text.
- Start timestamp.
- End timestamp.
- Exit code.
- Current working directory.

Command boundary detection may use Bash integration markers.

### 9.5 Output Capture Manager

Responsible for:

- Reading output from the PTY.
- Immediately forwarding output to the real terminal.
- Saving a copy of output.
- Associating output with the correct command.
- Handling large output.
- Handling terminal control sequences.

Important:

The captured data is terminal output as observed through the PTY. It may contain ANSI escape sequences and formatting characters.

### 9.6 Storage Layer

Responsible for:

- Creating the SQLite database.
- Creating tables.
- Inserting execution records.
- Querying records.
- Deleting records.
- Database migrations.
- Handling corrupted or locked databases.

### 9.7 Security and Privacy Layer

Responsible for:

- User-only database permissions.
- Avoiding root runtime privileges.
- Pause and resume functionality.
- Exclusion rules.
- Output size limits.
- Optional redaction.
- Safe deletion.

### 9.8 Shell Integration Layer

Responsible for:

- Starting `outlog` automatically.
- Preventing recursion.
- Detecting interactive Bash sessions.
- Avoiding interference with noninteractive scripts.
- Providing enable/disable functionality.
- Restoring the original shell configuration.

---

## 10. Automatic Startup Design

The main requirement is that users should not have to run:

```bash
outlog --start
```

The user should open a terminal normally and recording should already be active.

The preferred process tree is:

```text
Terminal emulator
    └── outlog
          └── bash
                ├── ls
                ├── pwd
                ├── terraform
                └── docker
```

The `outlog` process should start Bash inside a PTY.

### Recursion prevention

An environment variable should prevent recursive startup:

```text
OUTLOG_ACTIVE=1
```

Conceptually:

```bash
if interactive_shell && OUTLOG_ACTIVE is not set:
    export OUTLOG_ACTIVE=1
    exec /usr/bin/outlog --shell
fi
```

When `outlog` starts Bash, Bash inherits `OUTLOG_ACTIVE=1` and does not start another `outlog`.

### Important safety requirements

Automatic startup must:

- Apply only to interactive shells.
- Never affect shell scripts unintentionally.
- Avoid infinite recursion.
- Provide a recovery mechanism.
- Provide an emergency disable option.
- Preserve the user’s existing shell configuration.
- Avoid breaking SSH commands.
- Avoid breaking `sudo`.
- Avoid affecting system services.

A failed `outlog` startup should fall back to normal Bash if possible.

---

## 11. Shell Integration and Command Markers

`outlog` needs to know which output belongs to which command.

A possible design is to use Bash integration that emits hidden markers.

Conceptual markers:

```text
OUTLOG_COMMAND_START id=43
OUTLOG_COMMAND_END id=43 exit=0
```

The markers should:

- Be generated internally.
- Not be visible to the user.
- Be detected by the `outlog` PTY manager.
- Be removed before output is displayed or stored as normal command output.

The marker system should communicate:

- Command ID.
- Command text.
- Start time.
- End time.
- Exit status.
- Working directory.

Another option is to encode markers using special terminal control sequences.

The implementation must ensure that user command output containing similar text does not confuse the parser.

---

## 12. Data Model

SQLite is recommended for persistent storage.

Suggested database location:

```text
~/.local/share/outlog/outlog.db
```

The parent directory should be created with user-only permissions.

### Suggested schema

```sql
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
```

### Optional metadata table

```sql
CREATE TABLE IF NOT EXISTS metadata (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
```

Possible metadata:

```text
schema_version
installation_version
database_created_at
```

### Optional ignored-programs table

```sql
CREATE TABLE IF NOT EXISTS ignored_programs (
    name TEXT PRIMARY KEY,
    reason TEXT
);
```

Example ignored programs:

```text
nano
vim
nvim
emacs
top
htop
ssh
```

---

## 13. Execution Record Fields

Each recorded command should include:

### Command

Exactly what the user typed at the shell level, when possible.

Example:

```text
terraform apply
```

The system should not necessarily replace the command with a fully expanded version.

For example:

```bash
echo "$HOME"
```

should preferably remain:

```text
echo "$HOME"
```

### Output

Output captured from the PTY.

The first implementation may store the combined terminal stream.

A future implementation may separate:

- standard output
- standard error

### Working directory

Example:

```text
/home/alice/projects/infrastructure
```

### Start time

Use a consistent timestamp format, preferably UTC internally.

Example:

```text
2026-09-20T14:32:11Z
```

Display it in local time when appropriate.

### End time

Example:

```text
2026-09-20T14:34:03Z
```

### Duration

Store duration in milliseconds.

### Exit code

Examples:

```text
0       success
1       general failure
2       incorrect usage
130     terminated by Ctrl+C
```

### Recording status

Possible values:

```text
complete
output_skipped
output_truncated
interrupted
failed_to_record
```

---

## 14. Interactive Programs

Some programs should not have their terminal content recorded.

Examples:

```text
nano
vim
nvim
emacs
top
htop
ssh
less
```

For these programs, `outlog` should record metadata only:

```text
Command: nano notes.txt
Started: 2026-09-20 15:10:00
Finished: 2026-09-20 15:12:25
Exit code: 0
Output: [not recorded: excluded interactive program]
```

The contents typed into the editor must not be stored.

### Exclusion behavior

When an excluded program is detected:

1. Record the command and metadata.
2. Do not save terminal input.
3. Do not save terminal output.
4. Continue forwarding input/output normally.
5. Resume regular recording when the program exits.

The user should still be able to use the application normally.

### User configuration

Example configuration file:

```text
~/.config/outlog/ignore-programs.conf
```

Example:

```text
nano
vim
nvim
emacs
ssh
top
htop
```

The user should be able to add or remove exclusions:

```bash
outlog ignore-add nano
outlog ignore-remove nano
outlog ignore-list
```

---

## 15. Passwords and Sensitive Information

`outlog` must not record raw terminal keystrokes.

This means passwords typed into hidden prompts should not normally be captured as keyboard input.

Examples:

```bash
sudo command
ssh user@host
mysql -p
```

However, secrets may still appear in commands or output.

Examples:

```bash
curl -u user:password https://example.com
```

```bash
terraform apply -var="password=secret"
```

```bash
echo "$API_TOKEN"
```

Therefore, `outlog` should provide:

- A warning in documentation.
- Pause and resume.
- Record deletion.
- Output redaction.
- User-only file permissions.
- Configurable ignored commands.
- Optional maximum output size.

### Pause recording

```bash
outlog pause
```

### Resume recording

```bash
outlog resume
```

### Delete a record

```bash
outlog delete 43
```

### Clear all records

```bash
outlog clear
```

### File permissions

The database should be readable only by the user:

```text
-rw------- ~/.local/share/outlog/outlog.db
```

The configuration directory should also use restrictive permissions where appropriate.

`outlog` should not require root privileges during normal shell operation.

---

## 16. Command-Line Interface

### Display recent records

```bash
outlog -n 5
```

Equivalent long form:

```bash
outlog --last 5
```

### Display detailed record

```bash
outlog -d -c 5
```

Possible clearer long form:

```bash
outlog --details --id 5
```

### Search

```bash
outlog --search terraform
```

The search may search:

- Command text.
- Output text.
- Working directory.

### Show failed executions

```bash
outlog --failed
```

### Show records from a date

```bash
outlog --since yesterday
```

### Show records from a project directory

```bash
outlog --directory /home/alice/project
```

### Pause

```bash
outlog pause
```

### Resume

```bash
outlog resume
```

### Status

```bash
outlog status
```

Example:

```text
Automatic recording: enabled
Shell: bash
Database: /home/alice/.local/share/outlog/outlog.db
Database size: 14.2 MB
Recording: active
```

### Enable

```bash
outlog enable
```

### Disable

```bash
outlog disable
```

### Delete one record

```bash
outlog delete 5
```

### Clear all history

```bash
outlog clear
```

### Help

```bash
outlog --help
```

---

## 17. C++ Project Structure

Suggested structure:

```text
outlog/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── OUTLOG_PROJECT_SPEC.md
├── include/
│   └── outlog/
│       ├── cli.hpp
│       ├── database.hpp
│       ├── pty_session.hpp
│       ├── process_manager.hpp
│       ├── command_tracker.hpp
│       ├── output_capture.hpp
│       ├── signal_manager.hpp
│       └── configuration.hpp
├── src/
│   ├── main.cpp
│   ├── cli.cpp
│   ├── database.cpp
│   ├── pty_session.cpp
│   ├── process_manager.cpp
│   ├── command_tracker.cpp
│   ├── output_capture.cpp
│   ├── signal_manager.cpp
│   └── configuration.cpp
├── shell/
│   └── bash-integration.sh
├── tests/
│   ├── test_database.cpp
│   ├── test_cli.cpp
│   ├── test_configuration.cpp
│   └── test_record_queries.cpp
├── packaging/
│   └── debian/
│       ├── control
│       ├── rules
│       ├── install
│       ├── postinst
│       ├── prerm
│       └── conffiles
└── docs/
    ├── architecture.md
    ├── security.md
    └── troubleshooting.md
```

---

## 18. Recommended C++ Technologies

### Language

```text
C++20
```

C++17 is also acceptable if broader compatibility is required.

### Build system

```text
CMake
```

### Database

```text
SQLite3
```

### Testing

Choose one:

```text
GoogleTest
Catch2
```

### CLI parsing

Start with manual parsing or use:

```text
CLI11
```

### Linux APIs

Potential headers:

```cpp
#include <pty.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
```

---

## 19. Build Commands

Example development workflow:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The resulting binary may be:

```text
build/outlog
```

Run it:

```bash
./build/outlog --help
```

---

## 20. Development Phases

### Phase 1: CLI and database

Implement:

- Database creation.
- Record insertion.
- Recent record query.
- Detailed record query.
- Search.
- Failed command filtering.
- Delete.
- Clear.

At this stage, records may be inserted using test code.

### Phase 2: Explicit command wrapper

Implement:

```bash
outlog-run command arguments...
```

Example:

```bash
outlog-run terraform plan
```

The wrapper should:

- Start the requested process.
- Capture output.
- Display output immediately.
- Save output.
- Record timestamps.
- Record exit code.
- Record working directory.

This phase provides a functional prototype before automatic shell integration.

### Phase 3: PTY-based shell supervisor

Implement:

```bash
outlog --shell
```

Expected behavior:

```text
outlog --shell
```

starts a normal Bash session inside a PTY.

The user should be able to run:

```bash
pwd
ls
terraform plan
nano file.txt
```

without noticeable differences.

### Phase 4: Command tracking

Add Bash markers or another reliable mechanism to identify:

- Command start.
- Command completion.
- Command text.
- Exit status.
- Working directory.

### Phase 5: Automatic startup

Configure interactive Bash sessions to start through `outlog`.

Add:

```bash
outlog enable
outlog disable
outlog status
```

Test recovery if `outlog` is unavailable or crashes.

### Phase 6: Privacy features

Implement:

- Pause.
- Resume.
- Ignore list.
- Output truncation.
- Record deletion.
- Secure permissions.
- Sensitive-data warnings.

### Phase 7: Debian package

Create a `.deb` package that installs:

```text
/usr/bin/outlog
/usr/share/outlog/
```

The package should configure automatic Bash integration only after user consent.

### Phase 8: Testing and documentation

Test:

```text
normal commands
failed commands
large output
Ctrl+C
Ctrl+Z
Ctrl+D
terminal resize
nested shells
sudo
ssh
nano
vim
top
less
shell scripts
terminal closure
system shutdown
package removal
package upgrade
```

---

## 21. Debian Package Design

The first package should target Debian-based systems.

Possible installed files:

```text
/usr/bin/outlog
/usr/share/doc/outlog/README.md
/usr/share/outlog/bash-integration.sh
/etc/profile.d/outlog.sh
```

The package should not grant permanent root privileges to `outlog`.

Root privileges should be used only during installation to:

- Install system files.
- Install the executable.
- Configure integration if selected.
- Create package metadata.

During normal use:

```text
outlog runs as the logged-in user
```

The database should be stored in the user’s home directory:

```text
~/.local/share/outlog/outlog.db
```

### Package installation behavior

The package may ask:

```text
Enable automatic Bash recording? [Y/n]
```

If yes:

- Enable the Bash integration.
- Preserve existing shell configuration.
- Avoid duplicate entries.
- Provide a disable command.

If no:

- Install the binary.
- Leave automatic recording disabled.

### Package removal behavior

Removing the package should:

- Stop automatic startup.
- Remove integration files installed by the package.
- Preserve the user’s database unless the user explicitly chooses to delete it.
- Restore the previous shell behavior.

Purging may optionally remove configuration and records after confirmation.

---

## 22. Security Requirements

Security is important because command output may contain sensitive information.

### Required protections

- Do not run `outlog` as root during normal use.
- Do not use `sudo` for each recorded command.
- Use restrictive file permissions.
- Do not record raw keyboard input.
- Provide pause and resume.
- Provide deletion.
- Avoid exposing records to other users.
- Validate all database input using prepared statements.
- Avoid shell injection in internal command execution.
- Handle symbolic links carefully.
- Do not overwrite arbitrary files.
- Limit output size to prevent unlimited database growth.

### Database permissions

Recommended:

```text
0700 for the outlog data directory
0600 for the SQLite database
```

### Output limits

The user may run commands that produce gigabytes of output.

`outlog` should support limits such as:

```text
Maximum output per command: 10 MB
Maximum total database size: configurable
```

If output is truncated, the record should clearly indicate:

```text
Output: truncated
```

---

## 23. Reliability Requirements

`outlog` must not make the user’s shell unusable.

If the recorder fails:

- The shell should attempt to continue normally.
- A failure should be reported clearly.
- The user should be able to disable automatic integration.
- The system should not enter an infinite restart loop.

The command should handle:

- Child-process crashes.
- Database locks.
- Disk-full conditions.
- Broken terminal connections.
- Terminal closure.
- System shutdown.
- Interrupted commands.
- Unexpected Bash termination.

---

## 24. Important Technical Limitations

### Output is terminal output

The output may contain:

- ANSI color sequences.
- Cursor movement sequences.
- Progress bars.
- Terminal redraw operations.
- Control characters.

The initial version may store raw PTY output.

A future version may provide a cleaned display mode.

### Not every command is perfectly separable

Shell features such as:

```bash
command1 | command2
```

```bash
command1 && command2
```

```bash
(command1; command2)
```

```bash
command1 &
```

make command-boundary tracking more complicated.

The first version should document how compound commands are represented.

### Nested shells

Commands such as:

```bash
bash
```

or:

```bash
ssh user@host
```

may create nested terminal environments.

The first version may record metadata and limit output capture for these programs.

### Passwords

`outlog` should not capture hidden password keystrokes, but secrets can still appear in typed command arguments or command output.

Users must be informed of this limitation.

---

## 25. Testing Plan

### Unit tests

Test:

- Database creation.
- Record insertion.
- Record retrieval.
- Search.
- Date filtering.
- Exit-code filtering.
- Record deletion.
- Configuration parsing.
- Ignore list parsing.
- Command-line argument parsing.

### Integration tests

Test:

- `outlog-run pwd`.
- `outlog-run false`.
- Commands with stdout.
- Commands with stderr.
- Commands with large output.
- Commands with spaces and quotes.
- Commands returning different exit codes.

### PTY tests

Test:

- Bash startup.
- Input forwarding.
- Output forwarding.
- Terminal resize.
- Ctrl+C.
- Ctrl+Z.
- Ctrl+D.
- Child process termination.
- Shell exit.

### Interactive application tests

Test:

```text
nano
vim
nvim
less
top
htop
ssh
sudo
python
bash
```

### Package tests

Test:

```bash
sudo apt install ./outlog.deb
outlog status
sudo apt remove outlog
sudo apt purge outlog
```

Test installation choices:

```text
enable automatic recording
disable automatic recording
cancel installation
upgrade installation
```

---

## 26. Future Compatibility Roadmap

### Version 1

```text
Linux
Bash
Debian/Ubuntu
C++
SQLite
```

### Version 2

```text
Zsh
Fedora/RHEL
Arch Linux
```

### Version 3

```text
macOS
Homebrew
macOS PTY implementation
```

### Version 4

```text
Fish
Other Unix-like systems
Additional package managers
```

The platform-dependent PTY and shell-integration code should be isolated behind interfaces.

Example:

```text
src/platform/linux_pty.cpp
src/platform/macos_pty.cpp
src/platform/pty_interface.hpp
```

---

## 27. Why This Is a Good College Project

This project demonstrates:

- C++ systems programming.
- Linux process management.
- Pseudo-terminal programming.
- Shell integration.
- Inter-process communication.
- Signal handling.
- File-system permissions.
- SQLite database design.
- CLI design.
- Debian packaging.
- Security and privacy considerations.
- Automated testing.
- Software architecture.

It is more technically substantial than a simple CRUD application because the main challenge involves operating-system behavior and terminal control.

Suggested academic title:

> Outlog: An Automatic Persistent Shell Execution Journal for Linux

Suggested project objective:

> To design and implement a native Linux command-line utility that automatically records shell commands, terminal output, execution timestamps, working directories, and exit codes, allowing users to inspect previous command executions without rerunning potentially unsafe operations.

---

## 28. Recommended First Release Scope

The first public release should support:

```text
Linux
Bash
Ubuntu/Debian
C++20
SQLite
Automatic startup
Command records
Terminal output
Exit codes
Timestamps
Working directory
Recent-history display
Detailed record display
Search
Failed-command filtering
Pause/resume
Basic interactive-program exclusions
Debian package
```

The first version should not attempt to support:

```text
Zsh
Fish
macOS
Windows
Every Linux distribution
Cloud synchronization
Web interface
Team sharing
```

A focused and reliable Bash/Linux release is more valuable than an incomplete cross-platform implementation.

---

## 29. Summary

`outlog` is a native terminal-only command that extends the functionality of traditional shell history.

Traditional history:

```text
command text
```

Outlog:

```text
command text
output
timestamp
duration
exit code
working directory
recording status
```

The intended experience is:

```text
sudo apt install outlog
        ↓
User grants permission
        ↓
User opens a terminal normally
        ↓
outlog starts automatically
        ↓
Bash starts inside outlog
        ↓
User works normally
        ↓
Commands and selected output are stored
        ↓
User later queries old execution records
```

The most important design principle is:

> `outlog` should be invisible during normal work and useful when historical command output is needed.

The recommended implementation stack is:

```text
C++20
CMake
SQLite
Linux PTY APIs
Bash integration
Debian packaging
Terminal-only interface
```