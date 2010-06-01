#!/bin/sh
# Killall for dhcp clients.

# Use [] in sed /address/ to avoid matching sed command itself in ps output
pids=$(ps ax | sed -n '/[u]dhcpc\|[d]hclient\|[p]ump/s/^[ ]*\([0-9]*\).*/\1/p')

for pid in $pids; do
  if kill -0 $pid 2>/dev/null; then
    kill -TERM $pid
    sleep 1
    # Still alive? Die!
    if kill -0 $pid 2>/dev/null; then
      kill -KILL $pid
    fi
  fi
done
