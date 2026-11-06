#!/bin/bash

# outlog bash integration script
# This script should be sourced by bash inside the PTY session.
# It uses the DEBUG trap to detect when a command starts,
# and PROMPT_COMMAND to detect when it ends, emitting OSC (Operating System Command) sequences
# that outlog's PtySession will parse and intercept.

# Custom OSC sequence: \e]1337;Outlog;Action;... \a
# Start: \e]1337;Outlog;Start;<working_directory>\a
# End: \e]1337;Outlog;End;<exit_code>;<command>\a

# The DEBUG trap also fires for everything PROMPT_COMMAND runs, so it may only
# start a record when armed. __outlog_arm runs last in PROMPT_COMMAND, which
# means the next command to reach the trap is the one the user typed.
__outlog_preexec() {
    [ -n "${__outlog_armed:-}" ] || return 0
    [ -z "${COMP_LINE:-}" ] || return 0
    # Pressing Enter on an empty line goes straight back to PROMPT_COMMAND.
    [ "$BASH_COMMAND" != "__outlog_precmd" ] || return 0

    __outlog_armed=
    __outlog_cmd_started=1
    printf "\033]1337;Outlog;Start;%s\007" "$PWD"
}

__outlog_precmd() {
    local exit_code=$?
    local hist_entry hist_num full_cmd
    hist_entry=$(HISTTIMEFORMAT= history 1)
    read -r hist_num _ <<< "$hist_entry"
    # Get the most recent command from history
    full_cmd=$(printf '%s\n' "$hist_entry" | sed -e "s/^[ ]*[0-9]*[ ]*//")

    # A command that is only a ( subshell ) never reaches the DEBUG trap, so it
    # shows up here as a new history entry with no Start. Its output has already
    # gone past, but the command and exit code can still be recorded. Comments
    # also add history entries without running anything.
    if [ -n "${__outlog_armed:-}" ] && [ -z "${__outlog_cmd_started:-}" ] \
        && [ "$hist_num" != "${__outlog_hist_num:-}" ] && [ "${full_cmd:0:1}" != "#" ]; then
        __outlog_cmd_started=1
        printf "\033]1337;Outlog;Start;%s\007" "$PWD"
    fi

    __outlog_armed=
    __outlog_hist_num=$hist_num
    if [ -n "${__outlog_cmd_started:-}" ]; then
        printf "\033]1337;Outlog;End;%s;%s\007" "$exit_code" "$full_cmd"
        unset __outlog_cmd_started
    fi
}

__outlog_arm() {
    __outlog_armed=1
}

trap '__outlog_preexec' DEBUG

# Bash 5.1+ allows PROMPT_COMMAND to be an array. Newlines keep the scalar form
# valid when an existing value ends in a semicolon.
if [[ "$(declare -p PROMPT_COMMAND 2>/dev/null)" == "declare -a"* ]]; then
    PROMPT_COMMAND=(__outlog_precmd "${PROMPT_COMMAND[@]}" __outlog_arm)
else
    PROMPT_COMMAND=$'__outlog_precmd\n'"${PROMPT_COMMAND:-}"$'\n__outlog_arm'
fi

export OUTLOG_INTEGRATION_LOADED=1
