# Core regression tests

This project tests the Othello model and AI without starting DxLib.

## Run from Visual Studio

1. Open `UchinokoOthelloVersion2.sln`.
2. Build the solution.
3. Set `UchinokoOthelloCoreTests` as the startup project.
4. Run it without debugging.

A successful run prints:

```text
All core tests passed.
```

The test executable returns a non-zero exit code and prints the failed
invariant when a regression is detected.

## Covered behavior

- Initial position and initial legal moves
- Illegal and out-of-range moves
- Optimized legal-move and flip calculations compared with a slow
  eight-direction reference implementation
- Disc counts and bitboard overlap across deterministic complete games
- Pass and game-over behavior
- AI legality and configured search depth
- Exact endgame search compared with an independent minimax result
