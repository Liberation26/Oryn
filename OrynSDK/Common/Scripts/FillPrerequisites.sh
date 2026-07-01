#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
ORYN_APP="${SDK_ROOT}/Common/Bin/oryn"

if [ ! -x "${ORYN_APP}" ]; then
    "${SCRIPT_DIR}/BuildOryn.sh"
fi

exec "${ORYN_APP}" prerequisites "${1:-install}"
