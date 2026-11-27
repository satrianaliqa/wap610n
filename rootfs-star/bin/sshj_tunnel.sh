#!/bin/sh
# Auto-reconnecting ssh-j.com reverse SSH tunnel watchdog for WAP610N

WATCHDOG_PID_FILE="/tmp/sshj_tunnel.pid"
SSH_PID_FILE="/tmp/sshj_ssh.pid"
LOG_FILE="/tmp/sshj_tunnel.log"

get_conf_val() {
	key="$1"
	for conf in /mnt/jffs2/sys.conf /tmp/sys.conf; do
		if [ -f "$conf" ]; then
			val=$(awk -v k="$key" '
				$0 ~ "^[ \t]*" k "[ \t]*=" {
					idx = index($0, "=")
					v = substr($0, idx + 1)
					sub(/^[ \t]+/, "", v)
					sub(/[ \t\r]+$/, "", v)
					print v
				}
			' "$conf" 2>/dev/null | tail -n 1)
			if [ -n "$val" ]; then
				printf '%s' "$val"
				return 0
			fi
		fi
	done
	return 1
}

is_valid_ident() {
	case "$1" in
		""|*[!A-Za-z0-9_-]*)
			return 1
			;;
		*)
			return 0
			;;
	esac
}

is_valid_server() {
	case "$1" in
		""|*[!A-Za-z0-9.-]*)
			return 1
			;;
		*)
			return 0
			;;
	esac
}

is_valid_port() {
	case "$1" in
		""|*[!0-9]*)
			return 1
			;;
		*)
			[ "$1" -ge 1 ] && [ "$1" -le 65535 ] 2>/dev/null
			;;
	esac
}

read_cfg() {
	SSHJ_EN=$(get_conf_val "SSHJ_Enabled")
	[ -z "$SSHJ_EN" ] && SSHJ_EN=0
	SSHJ_USER=$(get_conf_val "SSHJ_User")
	SSHJ_HOST=$(get_conf_val "SSHJ_Host")
	[ -z "$SSHJ_HOST" ] && SSHJ_HOST="wap610n"
	SSHJ_SERVER=$(get_conf_val "SSHJ_Server")
	[ -z "$SSHJ_SERVER" ] && SSHJ_SERVER="ssh-j.com"
	SSHJ_PORT=$(get_conf_val "SSHJ_Port")
	[ -z "$SSHJ_PORT" ] && SSHJ_PORT="22"
}

is_alive() {
	pf="$1"
	if [ -f "$pf" ]; then
		pid=$(cat "$pf" 2>/dev/null)
		case "$pid" in
			""|*[!0-9]*)
				return 1
				;;
			*)
				kill -0 "$pid" 2>/dev/null && return 0
				;;
		esac
	fi
	return 1
}

is_ssh_established() {
	awk 'NR>1 && $4 == "01" && $3 ~ /:0016$/ { found=1; exit } END { exit !found }' /proc/net/tcp 2>/dev/null
}

kill_pidfile() {
	pf="$1"
	if [ -f "$pf" ]; then
		pid=$(cat "$pf" 2>/dev/null)
		case "$pid" in
			""|*[!0-9]*)
				;;
			*)
				kill "$pid" 2>/dev/null || true
				sleep 1
				kill -9 "$pid" 2>/dev/null || true
				;;
		esac
		rm -f "$pf" 2>/dev/null || true
	fi
}

do_stop() {
	kill_pidfile "$WATCHDOG_PID_FILE"
	kill_pidfile "$SSH_PID_FILE"
}

do_run() {
	read_cfg
	if [ "$SSHJ_EN" != "1" ] || ! is_valid_ident "$SSHJ_USER" || ! is_valid_ident "$SSHJ_HOST"; then
		exit 0
	fi
	if ! is_valid_server "$SSHJ_SERVER" || ! is_valid_port "$SSHJ_PORT"; then
		exit 0
	fi
	[ -x /usr/bin/ssh ] || exit 0

	echo "$$" > "$WATCHDOG_PID_FILE"
	SSH_PID=""
	trap '
		if [ -n "$SSH_PID" ]; then
			kill "$SSH_PID" 2>/dev/null || true
		fi
		rm -f "$SSH_PID_FILE" 2>/dev/null || true
		if [ "$(cat "$WATCHDOG_PID_FILE" 2>/dev/null)" = "$$" ]; then
			rm -f "$WATCHDOG_PID_FILE" 2>/dev/null || true
		fi
		exit 0
	' TERM INT EXIT

	while true; do
		read_cfg
		if [ "$SSHJ_EN" != "1" ] || ! is_valid_ident "$SSHJ_USER" || ! is_valid_ident "$SSHJ_HOST"; then
			break
		fi
		/usr/bin/ssh -yy -N -K 30 -I 0 -o ExitOnForwardFailure=yes \
			-R "${SSHJ_HOST}:${SSHJ_PORT}:127.0.0.1:22" \
			"${SSHJ_USER}@${SSHJ_SERVER}" </dev/null >"$LOG_FILE" 2>&1 &
		SSH_PID=$!
		echo "$SSH_PID" > "$SSH_PID_FILE"
		wait "$SSH_PID" 2>/dev/null || true
		SSH_PID=""
		rm -f "$SSH_PID_FILE" 2>/dev/null || true
		sleep 15
	done
}

do_start() {
	read_cfg
	[ "$SSHJ_EN" = "1" ] || exit 0
	is_valid_ident "$SSHJ_USER" || exit 0
	is_valid_ident "$SSHJ_HOST" || exit 0
	is_valid_server "$SSHJ_SERVER" || exit 0
	is_valid_port "$SSHJ_PORT" || exit 0
	[ -x /usr/bin/ssh ] || exit 0

	if is_alive "$WATCHDOG_PID_FILE"; then
		exit 0
	fi

	/bin/sshj_tunnel.sh _run </dev/null >/dev/null 2>&1 &
}

case "$1" in
	start)
		do_start
		;;
	stop)
		do_stop
		;;
	restart)
		do_stop
		sleep 1
		do_start
		;;
	status)
		if is_alive "$SSH_PID_FILE" && is_ssh_established; then
			echo "running"
			exit 0
		elif is_alive "$SSH_PID_FILE" || is_alive "$WATCHDOG_PID_FILE"; then
			echo "connecting"
			exit 0
		else
			echo "stopped"
			exit 1
		fi
		;;
	_run)
		do_run
		;;
	*)
		echo "Usage: $0 {start|stop|restart|status}"
		exit 1
		;;
esac
