// Checks WebAssembly build before published
// Published perft counts prove compiled move generation matches native one
const path = require('path');
const dir = path.resolve(process.argv[2] || 'dist-web');
const ChessBot = require(path.join(dir, 'chess_bot.js'));

function check(name, got, want) {
  if (got !== want) { console.error('FAIL ' + name + ': got ' + got + ', expected ' + want); process.exit(1); }
  console.log('ok   ' + name + ' = ' + got);
}

ChessBot().then((m) => {
  const position = m.cwrap('chess_position', 'number', ['string', 'string']);
  const bestmove = m.cwrap('chess_bestmove', 'string', ['number', 'number']);
  const legalmoves = m.cwrap('chess_legalmoves', 'string', []);
  const status = m.cwrap('chess_status', 'string', []);
  const perft = m.cwrap('chess_perft', 'number', ['number']);

  check('start position accepted', position('', ''), 1);
  check('perft(4) from the start', perft(4), 197281);
  check('legal moves at the start', legalmoves().split(' ').length, 20);

  // Kiwipete as standard test position for castling, en passant and promotion
  check('kiwipete accepted', position('r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1', ''), 1);
  check('perft(3) from kiwipete', perft(3), 97862);

  check('illegal move rejected', position('', 'e2e5'), 0);

  // Check engine answer with legal move
  check('game so far accepted', position('', 'e2e4 e7e5 g1f3'), 1);
  const legal = legalmoves().split(' '), move = bestmove(300, 64);
  check('best move is legal (' + move + ')', legal.includes(move), true);

  // Mate in one test is Qxf7#
  position('r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4', '');
  check('finds mate in one', bestmove(1000, 4), 'h5f7');
  position('r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4', 'h5f7');
  check('status after mate', status(), 'black checkmate');
  console.log('All checks passed.');
});
