#!/bin/bash

cd "$(dirname "$0")" || exit 1
if [ "$ARCHS" = "" ]; then
  ARCHS=X64
fi

package() {
  cp "$1/Loader.efi" "Binaries/Loader-${ARCHS}.efi" || exit 1
}

SELFPKG=LoaderPkg
RELPKG=LoaderPkg
SELFPKG_DIR=LoaderPkg
NO_ARCHIVES=1
TARGETS=NOOPT
RTARGETS=NOOPT

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
