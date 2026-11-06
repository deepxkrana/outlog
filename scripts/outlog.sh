#!/bin/sh
# /etc/profile.d/outlog.sh
# Starts outlog for interactive Bash login shells unless the user has run
# `outlog disable`. If outlog fails to start, the user keeps the normal shell.

if [ -n "${BASH_VERSION:-}" ] && [ -z "${OUTLOG_ACTIVE:-}" ] && [ -t 0 ] \
    && [ ! -f "$HOME/.config/outlog/disabled" ] && command -v outlog >/dev/null 2>&1; then
    case $- in
        *i*) outlog shell && exit ;;
    esac
fi
