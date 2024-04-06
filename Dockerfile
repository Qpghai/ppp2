FROM rebecca554owen/openppp2:meta as builder 
# 编译 OpenPPP2 
WORKDIR /app
RUN apt-get update \
    && git clone --single-branch --branch main https://github.com/liulilittle/openppp2 . \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc) \
    && cd ../bin

# 构建 OpenPPP2 镜像    
FROM ubuntu:latest
WORKDIR /app
COPY --from=builder /app/bin/ppp /app/
# 安装必要的工具包
RUN apt-get update && apt-get install -y --no-install-recommends \
    tzdata \
    iptables \
    dnsutils \
    iproute2 \
    net-tools \
    iputils-ping \
    && cp /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

ENTRYPOINT ["/app/ppp"]
