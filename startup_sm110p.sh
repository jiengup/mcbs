#!/bin/bash

export NVME_DEV=/dev/nvme0
export HUGEMEM=122880
export PCI_ALLOWED=10000:01:00.0
export VSCODE_INIT=1

./setup_env.sh
