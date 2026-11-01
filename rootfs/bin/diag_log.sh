#!/bin/sh

# Write important userspace diagnostics to the kernel ring buffer, serial
# console, and syslog. All runtime diagnostics should use this helper.
if [ "$#" -gt 0 ]; then
	msg="$*"
	printf '<4>wap610n: %s\n' "$msg" > /dev/kmsg 2>/dev/null || true
	printf '[wap610n] %s\n' "$msg" > /dev/console 2>/dev/null || true
	logger -t wap610n "$msg" 2>/dev/null || true
fi
