#!/bin/bash

cd "$(dirname "$0")" || exit 1
if [ "$ARCHS" = "" ]; then
  ARCHS=X64
fi

package() {
  if [ ! -d "$1"/../FV ]; then
    echo "Missing FV directory $1/../FV"
    exit 1
  fi

  if [ ! -f "$1"/../FV/OVMF.fd ]; then
    echo "Missing OVMF.fd directory in $1/../FV"
    exit 1
  fi

  cp "$1"/../FV/OVMF.fd "Binaries/OVMF-${ARCHS}.fd" || exit 1
}

if [ "$ARCHS" = "IA32" ]; then
  SELFPKG=OvmfPkgIa32
  RELPKG=OvmfIa32
elif [ "$ARCHS" = "X64" ]; then
  SELFPKG=OvmfPkgX64
  RELPKG=OvmfX64
else
  echo "Unsupported architecture $ARCHS"
fi
SELFPKG_DIR=OvmfPkg
NO_ARCHIVES=1
TARGETS=RELEASE
RTARGETS=RELEASE

export ARCHS
export SELFPKG
export RELPKG
export SELFPKG_DIR
export NO_ARCHIVES
export TARGETS
export RTARGETS

if [ "$(uname)" = "Linux" ]; then
  TOOLCHAINS=GCC5
  export TOOLCHAINS
fi

if [ ! -f "mtoc-mac64.sha256" ]; then
  curl -LO https://github.com/acidanthera/ocbuild/raw/master/external/mtoc-mac64.sha256 || exit 1
fi
MTOC_HASH=$(cat mtoc-mac64.sha256)
export MTOC_HASH

if [ ! -f "efibuild.sh" ]; then
  curl -LO https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh || exit 1
fi
. efibuild.sh
