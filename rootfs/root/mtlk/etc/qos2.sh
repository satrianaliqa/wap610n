		##tc qdisc del dev wlan1 root

		tc qdisc add dev wlan1 root handle 2: prio bands 4

		# TODO: Do you need to give different handles for the wlan1 tc queues?
		tc qdisc add dev wlan1 parent 2:1 handle 10: pfifo limit 100
		tc qdisc add dev wlan1 parent 2:2 handle 20: pfifo limit 1100
		tc qdisc add dev wlan1 parent 2:3 handle 30: pfifo limit 800
		tc qdisc add dev wlan1 parent 2:4 handle 40: pfifo limit 50

		tc filter add dev wlan1 parent 2:0 protocol ip prio 1 u32 match ip tos 224 0xff flowid 2:1
		tc filter add dev wlan1 parent 2:0 protocol ip prio 1 u32 match ip tos 192 0xff flowid 2:1
		tc filter add dev wlan1 parent 2:0 protocol ip prio 2 u32 match ip tos 160 0xff flowid 2:2
		tc filter add dev wlan1 parent 2:0 protocol ip prio 2 u32 match ip tos 128 0xff flowid 2:2
		tc filter add dev wlan1 parent 2:0 protocol ip prio 3 u32 match ip tos 0 0xff flowid 2:3
		tc filter add dev wlan1 parent 2:0 protocol ip prio 3 u32 match ip tos 96 0xff flowid 2:3
		tc filter add dev wlan1 parent 2:0 protocol ip prio 4 u32 match ip tos 32 0xff flowid 2:4
		tc filter add dev wlan1 parent 2:0 protocol ip prio 4 u32 match ip tos 64 0xff flowid 2:4
