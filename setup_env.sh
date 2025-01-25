#!/bin/bash
LIMITS_CONF="/etc/security/limits.conf"

set -e

git submodule update --init --recursive
sudo apt-get update
sudo apt-get install -y nvme-cli
sudo apt-get install -y git g++ make cmake \
                        libssl-dev libgflags-dev \
                        libprotobuf-dev libprotoc-dev \
                        protobuf-compiler libleveldb-dev 

if [ -d "third-party/spdk" ]; then
  sudo ./third-party/spdk/scripts/pkgdep.sh
else
  echo "Error: third-party/spdk not found"
  exit 1
fi

: ${NVME_DEV:=""}

if [ -n "$NVME_DEV" ]; then
  echo "Initializing NVMe disk..."
  sudo apt-get install nvme-cli
  sudo nvme format --reset -b 4096 "${NVME_DEV}n1"
  nvme format $NVME_DEV --namespace-id=1 --lbaf=4 --reset
fi

: ${PCI_ALLOWED:=""}
if [ -n "$PCI_ALLOWED" ]; then
  sudo PCI_ALLOWED=$PCI_ALLOWED ./third_party/spdk/scripts/setup.sh
fi