#!/usr/bin/env bash
# TouchDesigner integration test (macOS). Builds + copies the operator, launches
# TouchDesigner with tests/td/test.toe (which must contain the bootstrap Execute
# DAT — see TESTING.md), waits for the results.json sentinel the in-TD runner
# writes, then parses it and exits 0 (pass) or 1 (fail / timeout).
#
# Usage: ./run_td_tests.sh [--no-build] [--op NAME] [--timeout SEC]
#                          [--toe PATH] [--td APP_PATH]
set -euo pipefail

cd "$(dirname "$0")"

no_build=0
op="Asyncio1"
timeout_sec=180
toe="tests/td/test.toe"
td_app="${ASYNCIODAT_TD:-/Applications/TouchDesigner.app}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-build)      no_build=1; shift ;;
        --op)            op="$2"; shift 2 ;;
        --timeout)       timeout_sec="$2"; shift 2 ;;
        --toe)           toe="$2"; shift 2 ;;
        --td)            td_app="$2"; shift 2 ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done

toe_full="$(cd "$(dirname "$toe")" && pwd)/$(basename "$toe")"
td_bin="$td_app/Contents/MacOS/TouchDesigner"
results_path="$(dirname "$toe_full")/results.json"

if [[ ! -x "$td_bin" ]]; then
    echo "TouchDesigner not found at: $td_bin (set ASYNCIODAT_TD or pass --td)" >&2
    exit 1
fi

echo "TouchDesigner : $td_bin"
echo "Project       : $toe_full"
echo "Results file  : $results_path"

if [[ $no_build -eq 0 ]]; then
    echo "Building AsyncioDAT (compiles + copies to tests/td/Plugins/)..."
    ./build.sh
fi

rm -f "$results_path"

export ASYNCIODAT_RESULTS="$results_path"
export ASYNCIODAT_OP="$op"

echo "Launching TouchDesigner..."
"$td_bin" "$toe_full" &
td_pid=$!

deadline=$(( $(date +%s) + timeout_sec ))
found=0
while [[ $(date +%s) -lt $deadline ]]; do
    if [[ -f "$results_path" ]]; then found=1; break; fi
    if ! kill -0 "$td_pid" 2>/dev/null; then
        sleep 1
        [[ -f "$results_path" ]] && found=1
        break
    fi
    sleep 1
done

# The in-TD runner leaves TouchDesigner running; terminate it now.
kill "$td_pid" 2>/dev/null || true

if [[ $found -eq 0 ]]; then
    echo ""
    echo "FAILED: no results.json after ${timeout_sec}s." >&2
    echo "Is test.toe wired with the bootstrap Execute DAT? See TESTING.md." >&2
    exit 1
fi

echo ""
echo "==== AsyncioDAT integration results ===="
python3 - "$results_path" <<'PY'
import json, sys
d = json.load(open(sys.argv[1]))
for r in d["results"]:
    tag = "PASS" if r["passed"] else "FAIL"
    detail = (" " + r["detail"]) if r.get("detail") else ""
    print(f"  [{tag}] {r['name']}{detail}")
print(f"Total: {d['passed']} passed, {d['failed']} failed")
sys.exit(0 if d.get("success") else 1)
PY
