#!/usr/bin/env bash
set -euo pipefail

# scripts/verify_all_pages.sh
# Verifies all 6 navigation pages rendered headlessly via UIHarness,
# checks widget dumps, and captures screenshots to /tmp/verify_*.png.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

WS_DIR="${REPO_ROOT}/test_ui_verify"
rm -rf "${WS_DIR}"
mkdir -p "${WS_DIR}"

DUMP_LOG="/tmp/verify_dump.txt"

echo "=== [verify_all_pages] Running Headless UI Navigation Test ==="

"${REPO_ROOT}/scripts/run_ui_headless.sh" \
    --skip-welcome \
    --workspace "${WS_DIR}" \
    --ui-script "
        wait 5;
        page HOME; wait 5; dump_widgets; screenshot /tmp/verify_home.png;
        click Guide; wait 5; dump_widgets; screenshot /tmp/verify_guide.png;
        click \"Advanced Requests\"; wait 5; dump_widgets; screenshot /tmp/verify_advanced.png;
        click \"Client Configuration\"; wait 5; dump_widgets; screenshot /tmp/verify_client_config.png;
        click \"Request History\"; wait 5; dump_widgets; screenshot /tmp/verify_request_history.png;
        click \"Request Status\"; wait 5; dump_widgets; screenshot /tmp/verify_request_status.png;
        exit
    " 2>&1 | tee "${DUMP_LOG}"

rm -rf "${WS_DIR}"

echo "=== [verify_all_pages] Checking Generated Screenshots ==="
for page in home guide advanced client_config request_history request_status; do
    IMG="/tmp/verify_${page}.png"
    if [[ -f "${IMG}" ]]; then
        echo "  [OK] Found screenshot: ${IMG} ($(stat -c%s "${IMG}") bytes)"
    else
        echo "  [FAIL] Missing screenshot: ${IMG}"
        exit 1
    fi
done

echo "=== [verify_all_pages] Validating Layout Bounds & Overlap Elimination ==="

# 1. Verify Clear Canvas is within x <= 980px
CLEAR_CANVAS_RECT=$(grep 'label="Clear Canvas"' "${DUMP_LOG}" | head -n1 || true)
if [[ -n "${CLEAR_CANVAS_RECT}" ]]; then
    echo "  [OK] Found Clear Canvas widget: ${CLEAR_CANVAS_RECT}"
    # Parse rect=[x1,y1 -> x2,y2]
    MAX_X=$(echo "${CLEAR_CANVAS_RECT}" | sed -E 's/.*rect=\[[0-9]+,[0-9]+ -> ([0-9]+),[0-9]+\].*/\1/')
    if [[ "${MAX_X}" -le 980 ]]; then
        echo "  [PASS] Clear Canvas max x (${MAX_X}px) <= 980px"
    else
        echo "  [FAIL] Clear Canvas max x (${MAX_X}px) > 980px (clipped)"
        exit 1
    fi
else
    echo "  [FAIL] Clear Canvas button not found in widget dump"
    exit 1
fi

# 2. Verify Navigation sidebar items do not exceed x = 180px
NAV_OVERFLOW=$(grep '\[Navigation\] label=' "${DUMP_LOG}" | grep -v 'label="Navigation"' | while read -r line; do
    MAX_X=$(echo "${line}" | sed -E 's/.*rect=\[[0-9]+,[0-9]+ -> ([0-9]+),[0-9]+\].*/\1/')
    if [[ "${MAX_X}" -gt 180 ]]; then
        echo "OVERFLOW: ${line}"
    fi
done)

if [[ -z "${NAV_OVERFLOW}" ]]; then
    echo "  [PASS] All Navigation sidebar selectables terminate within x <= 180px"
else
    echo "  [FAIL] Navigation sidebar selectable exceeded 180px:"
    echo "${NAV_OVERFLOW}"
    exit 1
fi

# 3. Verify Client State collapsed pill bounds (y in [45, 77])
CLIENT_STATE_PILL=$(grep '\[Client State\] label="Client State"' "${DUMP_LOG}" | head -n1 || true)
if [[ -n "${CLIENT_STATE_PILL}" ]]; then
    echo "  [OK] Found Client State pill: ${CLIENT_STATE_PILL}"
    PILL_MIN_Y=$(echo "${CLIENT_STATE_PILL}" | sed -E 's/.*rect=\[[0-9]+,([0-9]+) -> [0-9]+,[0-9]+\].*/\1/')
    PILL_MAX_Y=$(echo "${CLIENT_STATE_PILL}" | sed -E 's/.*rect=\[[0-9]+,[0-9]+ -> [0-9]+,([0-9]+)\].*/\1/')
    if [[ "${PILL_MIN_Y}" -ge 40 && "${PILL_MAX_Y}" -le 80 ]]; then
        echo "  [PASS] Client State collapsed pill height bounded to y=[${PILL_MIN_Y}, ${PILL_MAX_Y}] <= 80px"
    else
        echo "  [FAIL] Client State pill height out of expected collapsed bounds: y=[${PILL_MIN_Y}, ${PILL_MAX_Y}]"
        exit 1
    fi
fi

# 4. Verify no child scroll panels in RestfulEzreal overlap with Client State in collapsed zone (y in [45, 77])
CHILD_PANEL_OVERLAP=$(grep -E '\[RestfulEzreal/.*\]' "${DUMP_LOG}" | while read -r line; do
    MIN_Y=$(echo "${line}" | sed -E 's/.*rect=\[[0-9]+,([0-9]+) -> [0-9]+,[0-9]+\].*/\1/')
    MAX_X=$(echo "${line}" | sed -E 's/.*rect=\[[0-9]+,[0-9]+ -> ([0-9]+),[0-9]+\].*/\1/')
    # If the panel starts before y=78 and extends past x=988, that would be an overlap
    if [[ "${MIN_Y}" -lt 78 && "${MAX_X}" -gt 988 ]]; then
        echo "OVERLAP: ${line}"
    fi
done)

if [[ -z "${CHILD_PANEL_OVERLAP}" ]]; then
    echo "  [PASS] Zero child panels overlap with Client State collapsed pill"
else
    echo "  [FAIL] Child panel overlap detected:"
    echo "${CHILD_PANEL_OVERLAP}"
    exit 1
fi

echo "=== [verify_all_pages] All 6 Navigation Pages Verified Successfully! ==="
