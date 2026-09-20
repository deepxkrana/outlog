#!/bin/sh
# /etc/profile.d/outlog.sh
# Automatically start outlog shell for interactive shells

if [ -z "$OUTLOG_ACTIVE" ] && [ -n "$PS1" ] && command -v outlog >/dev/null 2>&1; then
    exec outlog shell
fi
