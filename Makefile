SHELL = /bin/sh

BRIDGE := diyc0
ETH0 := $(shell ip -o -4 route show to default | awk '{print $$5}')

CC = gcc
CFLAGS = -std=c99 -Wall -Wno-unused-result -Werror -O2

.PHONY: all
all: nsexec

.PHONY: nsexec
nsexec: src/nsexec.c
	$(CC) $(CFLAGS) src/nsexec.c -o $@

.PHONY: clean
clean: net-clean
	rm -rf nsexec

.PHONY: setup
setup: net-setup

.PHONY: net-setup
net-setup:
	iptables -A FORWARD -i $(ETH0) -o veth -j ACCEPT || true
	iptables -A FORWARD -o $(ETH0) -i veth -j ACCEPT || true
	iptables -t nat -A POSTROUTING -s 172.16.0.0/16 -j MASQUERADE || true
	ip link add name $(BRIDGE) type bridge || true
	ip addr add dev $(BRIDGE) 172.16.0.1/24 || true
	ip link set $(BRIDGE) up || true
	iptables -A FORWARD -i $(ETH0) -o $(BRIDGE) -j ACCEPT || true
	iptables -A FORWARD -o $(ETH0) -i $(BRIDGE) -j ACCEPT || true
	iptables -A FORWARD -o $(BRIDGE) -i $(BRIDGE) -j ACCEPT || true

.PHONY: net-clean
net-clean:
	iptables -D FORWARD -i $(ETH0) -o veth -j ACCEPT || true
	iptables -D FORWARD -o $(ETH0) -i veth -j ACCEPT || true
	iptables -t nat -D POSTROUTING -s 172.16.0.0/16 -j MASQUERADE || true
	iptables -D FORWARD -i $(ETH0) -o $(BRIDGE) -j ACCEPT || true
	iptables -D FORWARD -o $(ETH0) -i $(BRIDGE) -j ACCEPT || true
	iptables -D FORWARD -o $(BRIDGE) -i $(BRIDGE) -j ACCEPT || true
	ip link delete $(BRIDGE) || true
