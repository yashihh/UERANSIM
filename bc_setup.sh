# namespace free5GC
sudo iptables -t mangle -F OUTPUT
sudo ifconfig upfgtp 10.60.0.1
sudo iptables -t mangle -I OUTPUT -d 224.0.1.129 -j TEE --gateway 10.60.0.0
sudo iptables -t mangle -A PREROUTING -i upfgtp -s 172.168.56.10 -j TEE --gateway 172.168.56.0
# namespace TSN
sudo ip netns exec TSN iptables -t mangle -F PREROUTING #flush
sudo ip netns exec TSN iptables -t mangle -A PREROUTING -i enp0s10 -s  172.168.56.10 -j TEE --gateway 10.60.0.0
sudo ip netns exec TSN iptables -t mangle -A PREROUTING -i uesimtun0 -s  172.168.56.5 -j TEE --gateway 172.168.56.0
sudo iptables -t mangle -L | grep TEE
sudo ip netns exec TSN iptables -t mangle -L | grep TEE
