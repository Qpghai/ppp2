FROM rebecca554owen/openppp2:meta as builder 
# 编译 OpenPPP2 
ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /app
RUN git clone --single-branch --branch main https://github.com/liulilittle/openppp2 . \
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
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    tzdata \
    iptables \
    iproute2 \
    dnsutils \
    iputils-ping \
    && cp /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

ENTRYPOINT ["/app/ppp"]
