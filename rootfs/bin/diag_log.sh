#!/bin/sh

# Write important userspace diagnostics to the kernel ring buffer, serial
# console, and syslog. All runtime diagnostics should use this helper.
if [ "$#" -gt 0 ]; then
	msg="$*"
	if [ -c /dev/kmsg ]; then
		printf '<4>wap610n: %s\n' "$msg" > /dev/kmsg 2>/dev/null || true
	fi
	if [ -S /dev/log ]; then
		logger -t wap610n "$msg" >/dev/null 2>&1 || true
	fi
fi
