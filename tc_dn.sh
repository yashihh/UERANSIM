# Redirect traffic from enp0s9(TSN Master) to upfgtp
if sudo tc qdisc show dev enp0s9 | grep -c "qdisc ingress"
then
    sudo tc qdisc del dev enp0s9 ingress
    echo "Delete qdisc ingress dev enp0s9"
else
    sudo tc qdisc add dev enp0s9 ingress
    sudo tc filter add dev enp0s9 parent ffff: protocol all u32 match u8 0 0 action mirred egress redirect dev upfgtp 
    sudo tc qdisc show dev enp0s9 | grep "qdisc ingress" 
    sudo tc filter show dev enp0s9 parent ffff:fff1
fi
