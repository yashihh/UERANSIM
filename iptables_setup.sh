# namespace TSN
echo "**** namespace TSN"
sudo iptables -t mangle -F PREROUTING #flush
sudo iptables -A PREROUTING -i enp0s9 -t mangle -j TEE --gateway 10.60.0.0
sudo iptables -t mangle -L | grep TEE
# namespace TSN2
echo "**** namespace TSN2"
sudo ip netns exec TSN iptables -t mangle -F PREROUTING #flush
sudo ip netns exec TSN ip addr add 172.168.56.2/24 dev enp0s10
sudo ip netns exec TSN iptables -A PREROUTING -i uesimtun0 -t mangle -j TEE --gateway 172.168.56.0
sudo ip netns exec TSN iptables -A PREROUTING -i enp0s10 -t mangle -j TEE --gateway 10.60.0.0
sudo ip netns exec TSN iptables -t mangle -L | grep TEE
