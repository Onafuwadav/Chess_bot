#!/usr/bin/env bash
# Builds browser bundle into dist-web/
#   chess_bot.js + chess_bot.wasm   is the engine
#   engine-worker.js                the worker the page talks to
#   index.html                      a small page to try it
#   version.json                    which commit it was built from
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${1:-dist-web}"
mkdir -p "$out"

em++ -std=c++17 -O2 -Wall -Wextra -Iengine \
  engine/board.cpp engine/movegen.cpp engine/eval.cpp engine/search.cpp engine/perft.cpp \
  play/web/main.cpp \
  -o "$out/chess_bot.js" \
  -sMODULARIZE=1 -sEXPORT_NAME=ChessBot -sENVIRONMENT=web,worker,node \
  -sEXPORTED_RUNTIME_METHODS=cwrap -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576

cp play/web/engine-worker.js play/web/index.html "$out/"
commit="$(git rev-parse HEAD 2>/dev/null || echo unknown)"
printf '{"commit":"%s","built":"%s"}\n' "$commit" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$out/version.json"
echo "Built $out (commit $commit)"
