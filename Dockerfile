FROM ubuntu:22.04

# 必要最小限のパッケージのみ
RUN apt-get update && apt-get install -y \
    gcc make iproute2 iptables \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
CMD ["/bin/bash"] 