#!/bin/bash
# Redirect traffic from enp0s9(TSN Master) to upfgtp
result=$(sudo ip netns exec TSN tc qdisc show dev enp0s10 | grep -c "qdisc ingress")
if [ "$result" = "1" ];
then
    sudo ip netns exec TSN tc qdisc del dev enp0s10 ingress
    echo "Delete qdisc ingress dev enp0s10"
else
    sudo ip netns exec TSN tc qdisc add dev enp0s10 ingress
    sudo ip netns exec TSN tc filter add dev enp0s10 parent ffff: protocol all u32 match u8 0 0 action mirred egress redirect dev uesimtun0 
    sudo ip netns exec TSN tc qdisc show dev enp0s10 | grep "qdisc ingress" 
    sudo ip netns exec TSN tc filter show dev enp0s10 parent ffff:fff1
fi