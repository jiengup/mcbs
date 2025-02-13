#!/bin/bash
LIMITS_CONF="/etc/security/limits.conf"

set -e

git submodule update --init --recursive
sudo apt-get update
sudo apt-get install -y jq
sudo apt-get install -y nvme-cli
sudo apt-get install -y clang clangd
sudo apt-get install -y git g++ make cmake \
                        libssl-dev libgflags-dev \
                        libprotobuf-dev libprotoc-dev \
                        protobuf-compiler libleveldb-dev 

if [ -d "third-party/spdk" ]; then
  echo "Found SPDK"
  sudo ./third-party/spdk/scripts/setup.sh reset
else
  echo "Error: third-party/spdk not found"
  exit 1
fi

if [ -d "third-party/spdk" ] && [ ! -d "third-party/spdk/build" ]; then
  sudo ./third-party/spdk/scripts/pkgdep.sh
  pushd third-party/spdk
  ./configure --enable-debug
  make -j
  popd
fi

if [ ! -e "/usr/bin/gdb-ori" ]; then
  sudo mv /usr/bin/gdb /usr/bin/gdb-ori
  sudo cp ./third-party/gdb/gdb /usr/bin/gdb
fi

: ${NVME_DEV:=""}
if [ -n "$NVME_DEV" ]; then
  while [ ! -e "${NVME_DEV}n1" ]; do
    echo "${NVME_DEV}n1 not found. Waiting..."
    sleep 1
  done
  echo "Initializing NVMe disk..."
  sudo nvme format --force --reset -b 4096 "${NVME_DEV}n1"
fi

: ${NVME_MD_FORMAT:=""}
if [ -n "$NVME_MD_FORMAT" ] && [ -n "${NVME_DEV}" ]; then
  sudo nvme format $NVME_DEV --force --namespace-id=1 --lbaf=4 --reset
fi

: ${PCI_ALLOWED:=""}
: ${HUGEMEM:=""}
if [ -n "$PCI_ALLOWED" ]; then
  sudo HUGEMEM=$HUGEMEM PCI_ALLOWED=$PCI_ALLOWED ./third-party/spdk/scripts/setup.sh || true
fi

: ${VSCODE_INIT:=""}
if [ -n "$VSCODE_INIT" ]; then
  code --install-extension ms-vscode.cpptools 
  code --install-extension llvm-vs-code-extensions.vscode-clangd 
  code --install-extension ms-vscode.cmake-tools
  code --install-extension GitHub.copilot
  code --install-extension GitHub.copilot-chat

  mkdir -p .vscode

  if [ ! -f .vscode/settings.json ]; then
      echo '{"cmake.generator": "Unix Makefiles"}' > .vscode/settings.json
  else
      jq '. + {"cmake.generator": "Unix Makefiles"}' .vscode/settings.json > temp.json && mv temp.json .vscode/settings.json
  fi
fi
