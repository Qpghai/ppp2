FROM openppp2:meta as builder 

# 构建 OpenPPP2
WORKDIR /app
RUN apt-get update && apt-get install -y --no-install-recommends \
    git clone --single-branch --branch main https://github.com/liulilittle/openppp2 . \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc) \
    && chmod +x ppp \
    && cd ../bin \

ENTRYPOINT ["/app/bin/ppp"]
