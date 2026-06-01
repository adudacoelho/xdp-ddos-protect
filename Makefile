IFACE ?= lo
PROG ?= xdp_ddos_protection
MAP ?= rate_limit_map

MONITOR_BIN ?= monitor

.PHONY: all compile clean attach detach dump

all: detach clean compile attach dump

compile: clean
# 	clang -O2 -g -Wall -target bpf \
# 	-I/usr/include/x86_64-linux-gnu \
# 	-c $(PROG).c -o $(PROG).o
	clang -O2 -g -target bpf -c $(PROG).c -o $(PROG).o
clean:
	rm -f xdp_ddos_protection.o
	rm -f $(MONITOR_BIN)
# 	rm -f $(PROG).o

attach:
	@echo "Attach step requires a privileged runner; run locally with: make attach IFACE=$(IFACE)"
	@echo "To actually attach on a privileged/self-hosted runner, run: sudo ip link set dev $(IFACE) xdp obj $(PROG).o sec xdp"

detach:
	@echo "Detach step requires a privileged runner; run locally with: make detach IFACE=$(IFACE)"
	@echo "To actually detach on a privileged/self-hosted runner, run: sudo ip link set dev $(IFACE) xdp off"

iface-inspect:
	@echo "Inspecting interface requires privileges; run locally: sudo ip link show $(IFACE)"

monitor: $(MONITOR_BIN)

$(MONITOR_BIN): monitor.c
	gcc -O2 -o $(MONITOR_BIN) monitor.c $(shell pkg-config --cflags --libs libbpf 2>/dev/null || echo "")

dump:
	@echo "Dumping BPF map (if loaded). On CI without privileges this may be empty."
	sudo bpftool map dump name $(MAP) || true
