FROM mcr.microsoft.com/devcontainers/cpp:2-debian13

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

RUN export DEBIAN_FRONTEND=noninteractive && apt update && apt install -y \
  tar unzip wget curl libcurl4 expat openssl make gcc g++ gettext libc6 \
  nfs-common libtool sqlite3 zlib1g-dev libssl-dev cmake ninja-build\
  libcurl4-openssl-dev sudo autoconf build-essential rapidjson-dev \
  texinfo binutils expat libelf-dev libdwarf-dev openssh-client ssh \
  dos2unix libxext-dev libxtst-dev libxt-dev libcups2-dev clang clang-16 libedit-dev \
  libxrender-dev zip bzip2 libopenmpi-dev vim gdb lldb libclang-dev libgtest-dev \
  rpm patch libtinfo6 cpio rpm2cpio libncurses6 strace net-tools swig;
