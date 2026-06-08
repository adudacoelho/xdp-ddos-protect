FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y \
    clang llvm libbpf-dev linux-tools-generic \
    make gcc gcc-multilib pkg-config iproute2 \
    hping3 iputils-ping && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /xdp
COPY xdp-projeto/xdp-ddos-protect-main/ .
RUN make compile
CMD ["sleep", "infinity"]
