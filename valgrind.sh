#!/bin/sh

cat << LIST >/tmp/dlmanagerlist
http://obiwan/iso/windows/xp/XP/AUTORUN.INF
http://obiwan/iso/windows/xp/XP/SETUP.EXE
LIST

valgrind ./dlmanager
