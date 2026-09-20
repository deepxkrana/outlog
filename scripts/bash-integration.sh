#!/bin/bash

# outlog bash integration script
# This script should be sourced by bash inside the PTY session.
# It uses the DEBUG trap to detect when a command starts,
# and PROMPT_COMMAND to detect when it ends, emitting OSC (Operating System Command) sequences
# that outlog's PtySession will parse and intercept.

# Custom OSC sequence: \e]1337;Outlog;Action;... \a
# Start: \e]1337;Outlog;Start\a
# End: \e]1337;Outlog;End;<exit_code>;<command>\a

__outlog_preexec() {
    if [ -z "${__outlog_cmd_started:-}" ]; then
        __outlog_cmd_started=1
        printf "\033]1337;Outlog;Start\007"
    fi
}

__outlog_precmd() {
    local exit_code=$?
    if [ -n "${__outlog_cmd_started:-}" ]; then
        local full_cmd
        # Get the most recent command from history
        full_cmd=$(HISTTIMEFORMAT= history 1 | sed -e "s/^[ ]*[0-9]*[ ]*//")
        printf "\033]1337;Outlog;End;%s;%s\007" "$exit_code" "$full_cmd"
        unset __outlog_cmd_started
    fi
}

trap '__outlog_preexec' DEBUG
PROMPT_COMMAND="__outlog_precmd; ${PROMPT_COMMAND:-}"

export OUTLOG_INTEGRATION_LOADED=1
