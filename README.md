# sirin_homework
deps:
libpcap-dev

How to use:
1. `make`  
2. `cd bin`  
3. `sudo ./sirin_sniffer start`  
4. Try other command.  
Daemon will continue work in background (except `erase` and `stop` commands).  
If daemon killed (SIGTERM) or stopped by command - it will save all collected data.  
After start daemon loads saved data (if it exists).
`erase` deletes all saved data and stops daemon

Command examples:
`sudo ./sirin_sniffer start`
`sudo ./sirin_sniffer show 8.8.8.8 count`
`sudo ./sirin_sniffer select iface lo`
`sudo ./sirin_sniffer stat`
`sudo ./sirin_sniffer stat eth0`
`sudo ./sirin_sniffer stop`
`sudo ./sirin_sniffer erase`


view daemon log: `tail -f /var/log/syslog | grep -a "sirin_sniffer"`

Default iface not ETH0, but default in OS (in my case it's `enp3s0`).
