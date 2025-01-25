#!/bin/bash
LIMITS_CONF="/etc/security/limits.conf"

set -e

git submodule update --init --recursive
sudo apt-get update
sudo apt-get install -y nvme-cli
sudo apt-get install -y git g++ make cmake \\
                        libssl-dev libgflags-dev \\
                        libprotobuf-dev libprotoc-dev \\
                        protobuf-compiler libleveldb-dev 

if ! grep -q "${USER}.*memlock" "$LIMITS_CONF"; then
  echo -e "\n${USER}    soft    memlock    unlimited" >> "$LIMITS_CONF"
  echo -e "${USER}    hard    memlock    unlimited" >> "$LIMITS_CONF"
fi

if [ -d "third_party/spdk" ]; then
  ./third_party/spdk/scripts/pkgdep.sh
else
  echo "Error: third_party/spdk not found"
  exit 1
fi