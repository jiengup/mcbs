#!/bin/bash

export NVME_DEV=/dev/nvme0
export NVME_MD_FORMAT=1
export HUGEMEM=122880
export PCI_ALLOWED=0000:c1:00.0
export VSCODE_INIT=1

./setup_env.sh
