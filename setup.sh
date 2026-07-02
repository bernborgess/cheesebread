set -e # Stop on first error
set -v # Verbose

# Architecture name
export ARCH=$(uname -m)
#export ARCH=x86_64 or aarch64

export WORKSPACE="$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd)"
# Static Library Compilation
export BUILD_ROOT=$WORKSPACE/buildtools
mkdir -p $BUILD_ROOT
tmp_cpus=$(grep -w processor /proc/cpuinfo | wc -l);

# Compile ncurses static, skip if ./buildtools/ncurses-6.5/usr exists
if [ ! -d "$BUILD_ROOT/ncurses-6.5/usr" ]; then
cd $BUILD_ROOT;
[ ! -d "ncurses-6.5" ] && \
  git clone https://gitcode.com/openharmony/third_party_ncurses.git \
  -b OpenHarmony-v6.0-Release ncurses-6.5;

cd ncurses-6.5;
./configure --with-termlib CC=clang CXX=clang++ CFLAGS=-fPIC CPPFLAGS=-fPIC \
  CFLAGS="-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack" \
  CXXFLAGS="-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack" \
  --with-terminfo-dirs=/etc/terminfo:/lib/terminfo:/usr/share/terminfo \
  --disable-widec --disable-overwrite --disable-root-environ;
make -j ${tmp_cpus};
make install DESTDIR=${BUILD_ROOT}/ncurses-6.5;
fi

# Compile libedit 3.1 static, skip if ./buildtools/libedit-3.1 exists
if [ ! -d "$BUILD_ROOT/libedit-3.1" ]; then
cd $BUILD_ROOT;
[ ! -d "third_party_libedit" ] && \
  git clone https://gitcode.com/openharmony/third_party_libedit.git \
  -b OpenHarmony-5.0.0-Release;
cd third_party_libedit && tar xf libedit-20210910-3.1.tar.gz;
cd libedit-20210910-3.1;
./configure --with-pic --enable-shared=no --prefix=${BUILD_ROOT}/libedit-3.1;
make -j ${tmp_cpus};
make install;
fi

# https://gitcode.com/Cangjie/cangjie_build/blob/main/doc_en/linux.md#24-set-environment-variables
export OPENSSL_PATH=/usr/lib/${ARCH}-linux-gnu/libssl.so
if [ ! -f "$OPENSSL_PATH" ]; then
    echo "$OPENSSL_PATH does not exist. Please fix your libssl installation."
    exit 1
fi
export LD_LIBRARY_PATH=$OPENSSL_PATH:$LD_LIBRARY_PATH

# https://gitcode.com/Cangjie/cangjie_build/blob/main/docs/env.md
# ? Looks like they want clang-16 (support)
if [ ! -f "/usr/lib/llvm-18/bin/clang-18" ]; then
    echo "clang-18 does not exist. Please fix your clang installation."
    exit 1
fi
export PATH=/usr/lib/llvm-18/bin:$PATH; # clang-18 is available?

# Cangjie SDK version number
export CANGJIE_VERSION=1.0.0
# Stdx version number
export STDX_VERSION=1

if [ "$ARCH" = "x86_64" ]; then
export SDK_NAME=linux-x64
else
export SDK_NAME=linux-aarch64
fi

# Build process
export CMAKE_PREFIX_PATH=$BUILD_ROOT/libedit-3.1:$BUILD_ROOT/ncurses-6.5/usr;

# Initilialize submodules if needed.
cd $WORKSPACE
#git submodule update --init --recursive 
# ! The main branch of cangjie_runtime has compilation issues,
# using their branch "release/OpenHarmony-release-6.0.2"

# Execute build
if [ ! -f "$WORKSPACE/cangjie_compiler/output/envsetup.sh" ]; then
cd $WORKSPACE/cangjie_compiler;
python3 build.py clean; # TODO: Bypass this for faster recompilation.
python3 build.py build -t debug \
  -v ${CANGJIE_VERSION} \
  --no-tests \
  --target-lib=$BUILD_ROOT/ncurses-6.5/usr/lib \
  --build-cjdb;
python3 build.py install;
fi

source $WORKSPACE/cangjie_compiler/output/envsetup.sh
cjc -v

# Build cangjie runtime
# ! SKIP IF DONE
cd $WORKSPACE/cangjie_runtime/runtime;
python3 build.py clean;
python3 build.py build -t debug -v ${CANGJIE_VERSION};
python3 build.py install;
cp -R output/common/linux_debug_${ARCH}/{lib,runtime} $WORKSPACE/cangjie_compiler/output;

echo "HERE AND OK"
exit
# Build cangjie stdlib
# ! SKIP IF DONE
cd $WORKSPACE/cangjie_runtime/stdlib;
python3 build.py clean;
python3 build.py build -t debug \
  --target-lib=$WORKSPACE/cangjie_runtime/runtime/output \
  --target-lib=$OPENSSL_PATH;
python3 build.py install;
cp -R output/* ../../cangjie_compiler/output/;

# cjpm
echo "HERE"
cd ${WORKSPACE}/cangjie_tools/cjpm/build;
python3 build.py clean;
python3 build.py build -t debug --set-rpath \$ORIGIN/../../runtime/lib/linux_${ARCH}_cjnative;
python3 build.py install;
mkdir -p ${WORKSPACE}/cangjie_compiler/output/tools/config;
cp ${WORKSPACE}/cangjie_tools/cjpm/dist/cjpm   ${WORKSPACE}/cangjie_compiler/output/tools/bin;
mv ${WORKSPACE}/cangjie_tools/cjpm/dist/*.toml ${WORKSPACE}/cangjie_compiler/output/tools/config;

