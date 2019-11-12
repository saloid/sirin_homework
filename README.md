# sirin_homework
deps:
libpcap-dev

How to use:
1. Compile `make`

view daemon log: `tail -f /var/log/syslog | grep -a "sirin_sniffer"`

Default iface not ETH0, but that which is default in OS (in my case it's `enp3s0`)