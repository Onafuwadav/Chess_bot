// Web Worker that owns engine, page talks to it with messages,

//   const engine = new Worker('https://<user>.github.io/Chess_bot/engine-worker.js');
//   engine.onmessage = (e) => console.log(e.data);
//   engine.postMessage({ id: 1, fen: '', moves: 'e2e4', timeMs: 1000 });

// Lets page on another site start worker from a small wrapper script
const BASE = self.BUNDLE_BASE || self.location.href.replace(/[^/]*$/, '');
importScripts(BASE + 'chess_bot.js');

const ready = ChessBot({ locateFile: (file) => BASE + file }).then(async (m) => {
  let version = null;
  try { version = await (await fetch(BASE + 'version.json', { cache: 'no-cache' })).json(); } catch (e) { /* optional */ }
  postMessage({ ready: true, version });
  return {
    position: m.cwrap('chess_position', 'number', ['string', 'string']),
    bestmove: m.cwrap('chess_bestmove', 'string', ['number', 'number']),
    legalmoves: m.cwrap('chess_legalmoves', 'string', []),
    status: m.cwrap('chess_status', 'string', []),
    nodes: m.cwrap('chess_nodes', 'number', []),
    perft: m.cwrap('chess_perft', 'number', ['number']),
  };
});

onmessage = async (e) => {
  const req = e.data || {}, engine = await ready;
  const ok = engine.position(req.fen || '', req.moves || '') === 1;
  const out = { id: req.id, ok, bestmove: null, legal: [], status: '', nodes: 0 };
  if (ok) {
    const legal = engine.legalmoves();
    out.legal = legal ? legal.split(' ') : [];
    out.status = engine.status();
    if (req.timeMs > 0 && out.legal.length) {
      out.bestmove = engine.bestmove(req.timeMs, req.maxDepth || 64);
      out.nodes = engine.nodes();
    }
    if (req.perft > 0) {
      const start = performance.now();
      out.perft = engine.perft(req.perft);
      out.perftMs = Math.round(performance.now() - start);
    }
  }
  postMessage(out);
};
