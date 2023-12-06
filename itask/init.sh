#!/bin/bash
sudo ip link add br0 type bridge
sudo ip tuntap add dev tap0 mode tap
sudo ip link set tap0 up promisc on
sudo ip link set tap0 master br0
sudo ip addr add 192.160.144.1/24 broadcast 192.160.144.255 dev br0
sudo ip link set dev br0 up
sudo ip link set dev tap0 up

