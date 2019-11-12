# sirin_homework
deps:
libpcap-dev

Show daemon log: `tail -f /var/log/syslog | grep -a "sirin_sniffer"`

Default iface not ETH0, but that which is default in OS (in my case it's `enp3s0`)