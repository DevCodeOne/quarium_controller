FROM alpine:3.11.3 AS build

# Install dependencies for building the application
# To build general software in alpine
RUN apk add --no-cache build-base gcc abuild binutils
# Dependencies to build the the application itself
RUN apk add --no-cache python3 py3-setuptools cmake g++ git autoconf autoconf-archive libtool automake linux-headers
# To use bash, etc
RUN apk add --no-cache bash util-linux pciutils usbutils coreutils binutils findutils grep

RUN pip3 install 'conan==1.22.2'

WORKDIR /opt/aq_controller

SHELL ["/bin/bash", "-c"]
RUN rm /bin/sh
RUN ln -s /bin/bash /bin/sh
COPY ./include/ ./include/
COPY ./src/ ./src/
COPY ./tests/ ./tests/
COPY ./conanfile.py conanfile.py
COPY ./CMakeLists.txt CMakeLists.txt

WORKDIR build

RUN conan config install 'https://github.com/DevCodeOne/conan_config.git'
RUN conan profile new default --detect
RUN conan profile update "settings.compiler.libcxx"="libstdc++11" default
RUN conan config set general.cpu_count=2
# build everything, otherwise there might be issues related to the fact the alpine uses musl instead of libc
RUN conan install .. -s build_type=Release --build
RUN conan build ..
RUN conan package ..

FROM alpine:3.11.3 AS runner
RUN apk add --no-cache g++
WORKDIR /opt/aq_controller
COPY --from=build /opt/aq_controller/build/bin/quarium_controller .
ENTRYPOINT ["/bin/ash"]

