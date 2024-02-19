# namespace TSN
echo "**** namespace TSN"
sudo iptables -t mangle -F PREROUTING #flush
sudo iptables -t mangle -A PREROUTING -i enp0s9 -s 172.168.56.3 -j TEE --gateway 10.60.0.0
sudo iptables -t mangle -A PREROUTING -i upfgtp -s 172.168.56.10 -j TEE --gateway 172.168.56.0
sudo iptables -t mangle -L | grep TEE
# namespace TSN2
echo "**** namespace TSN2"
sudo ip netns exec TSN iptables -t mangle -F PREROUTING #flush
sudo ip netns exec TSN ip route add 10.60.0.0/24 dev uesimtun0
sudo ip netns exec TSN iptables -t mangle -A PREROUTING -i enp0s10 -s  172.168.56.10 -j TEE --gateway 10.60.0.0
sudo ip netns exec TSN iptables -t mangle -A PREROUTING -i uesimtun0 -s  172.168.56.3 -j TEE --gateway 172.168.56.0
sudo ip netns exec TSN iptables -t mangle -L | grep TEE




