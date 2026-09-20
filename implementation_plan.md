# Outlog Implementation Plan

Based on the provided `OUTLOG_PROJECT_SPEC.md`, here is the plan to develop `outlog`, broken down into logical phases and parts for easier and systematic development. We will tackle each phase sequentially, ensuring a solid foundation before adding complexity.

## Proposed Development Phases

### Phase 1: Foundation (CLI and Database)
**Goal:** Establish the C++ project structure, build system, SQLite database schema, and the Command-Line Interface (CLI).
* **Part 1:** Set up `CMakeLists.txt` and basic project structure (`src/`, `include/outlog/`, `tests/`).
* **Part 2:** Implement SQLite database management (schema creation, opening/closing connections).
* **Part 3:** Implement the CLI argument parser (`outlog -n 5`, `outlog --search`, `outlog clear`, etc.).
* **Part 4:** Wire the CLI to perform dummy database inserts and actual queries (insert execution, query recent, query detailed, search, clear, delete).

### Phase 2: Explicit Command Wrapper (`outlog-run`)
**Goal:** Create a functional prototype that wraps a single command without full shell integration.
* **Part 1:** Implement the `ProcessManager` to fork and execute a single command (e.g., `outlog-run terraform plan`).
* **Part 2:** Capture the standard output/error from the process.
* **Part 3:** Save the execution details (command, output, timestamps, exit code, working directory) into the database.
* **Part 4:** Stream the captured output immediately back to the user's terminal while saving it.

### Phase 3: PTY-Based Shell Supervisor
**Goal:** Transition from a simple process wrapper to a fully functional pseudo-terminal (PTY) session that supervises a Bash shell.
* **Part 1:** Implement `PtySession` to allocate a pseudo-terminal and start `bash` inside it.
* **Part 2:** Connect the real terminal's standard input/output to the PTY master.
* **Part 3:** Implement proper signal handling (resizing the terminal, forwarding signals).
* **Part 4:** Ensure interactive programs (like `vim`, `nano`) work smoothly within the PTY.

### Phase 4: Command Tracking (Bash Integration)
**Goal:** Track individual commands executed inside the PTY Bash session.
* **Part 1:** Develop a Bash integration script (`bash-integration.sh`) using `$PROMPT_COMMAND` or `DEBUG` traps to emit hidden markers when a command starts and ends.
* **Part 2:** Update `OutputCapture` to detect these markers from the PTY stream.
* **Part 3:** Slice the PTY stream by command boundaries and save each command's execution record into the database.

### Phase 5: Automatic Startup & Shell Integration
**Goal:** Make `outlog` start automatically when a new terminal opens, preventing recursion.
* **Part 1:** Implement recursion prevention (e.g., checking `OUTLOG_ACTIVE` environment variable).
* **Part 2:** Add configuration commands (`outlog enable`, `outlog disable`, `outlog status`).
* **Part 3:** Manage the user's `.bashrc` or system profile to seamlessly start `outlog` if enabled.

### Phase 6: Privacy and Limitations Handling
**Goal:** Handle edge cases and implement privacy controls to protect sensitive data.
* **Part 1:** Implement `outlog pause` and `outlog resume` states.
* **Part 2:** Implement the ignored programs list (e.g., `nano`, `vim`, `ssh`) where only metadata is recorded and output is skipped.
* **Part 3:** Implement output size truncation to prevent database bloat from massive command outputs.

### Phase 7: Packaging and Deployment (Debian Package)
**Goal:** Distribute the utility as a `.deb` package.
* **Part 1:** Create `packaging/debian/` control files and rules.
* **Part 2:** Implement post-installation scripts (`postinst`) to ask the user if they want to enable automatic recording.
* **Part 3:** Handle safe uninstallation, leaving user databases intact unless explicitly purged.

### Phase 8: Hardening and Testing
**Goal:** Ensure the system handles edge cases gracefully.
* **Part 1:** Write comprehensive automated unit tests for all components.
* **Part 2:** Perform manual testing for edge cases: `Ctrl+C`, system shutdowns, heavily nested shells, etc.

### Phase 9: Global Auto-Start and CLI Enhancements
**Goal:** Implement global auto-start upon installation and enhance CLI capabilities for viewing history.
* **Part 1:** Create an `/etc/profile.d/outlog.sh` script to automatically wrap interactive shells globally without requiring manual `outlog enable`.
* **Part 2:** Update `CMakeLists.txt` and Debian packaging to install the `/etc/profile.d` script globally.
* **Part 3:** Update `cli.cpp` so that running `outlog` without arguments shows **all** stored historical commands.
* **Part 4:** Implement `outlog -d` (details flag) to show all historical commands along with their full `stdout`/`stderr` outputs when no subcommand is given.

## User Review Required
Please review Phase 9. Installing to `/etc/profile.d/` will make `outlog` run automatically for **all users** on the system whenever they open an interactive terminal. Is this the intended behavior for the system-wide `.deb` package?

---

## User Review Required

Before we begin coding, please review the above plan. 
Does this phased approach align with your expectations for the project? 

If this looks good, we will start with **Phase 1** by setting up the project directory in your workspace and writing the `CMakeLists.txt` and initial C++ headers. 

> [!TIP]
> We recommend using the directory `/Users/deepakrana/.gemini/antigravity-ide/scratch/outlog` as your active workspace for this project.

## Open Questions

1. Should we use an external library like **CLI11** for argument parsing as suggested in the spec, or start with manual parsing to reduce dependencies?
2. Are you comfortable with us creating the project directory at `/Users/deepakrana/.gemini/antigravity-ide/scratch/outlog`?
