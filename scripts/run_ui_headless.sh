#!/usr/bin/env bash
set -euo pipefail

# Runs Restful-Ezreal inside an isolated Xvfb virtual framebuffer (1280x720x24)
# so UI tests, widget dumps, and screenshots never touch the user's desktop display.
#
# Usage:
#   ./scripts/run_ui_headless.sh --skip-welcome --ui-script "wait 5; dump_widgets; screenshot /tmp/ui.png; exit"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN_PATH="${REPO_ROOT}/build/Restful-Ezreal"

if [[ ! -x "${BIN_PATH}" ]]; then
    echo "[run_ui_headless] Building Restful-Ezreal..."
    cmake --build "${REPO_ROOT}/build" --target Restful-Ezreal -j"$(nproc)"
fi

cd "${REPO_ROOT}"
exec xvfb-run -a -s "-screen 0 1280x720x24" "${BIN_PATH}" "$@"
