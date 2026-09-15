# Policy training-data generator

`UchinokoOthelloTrainingData` generates training data for the
NeuralNetDxlib Othello Policy model. Each recorded position is searched once
per legal move with a common full window and depth. The v2 CSV contains 128
side-to-move-relative board features followed by 64 move scores; illegal
squares are written as `x`.

The analyzer deliberately bypasses the opening book. Self-play still uses the
normal AI outside the recorded phase, so opening positions remain inexpensive.
Positions with only one legal move are excluded by default because they do not
teach move ordering.

Run a Release x64 build from the repository root:

```powershell
build\x64\Release\UchinokoOthelloTrainingData.exe `
  --output othello_policy_training.csv `
  --positions 10000 `
  --depth 5 `
  --exact-endgame-empty 12 `
  --exploration-percent 30 `
  --min-empty 12 `
  --max-empty 52 `
  --minimum-legal-moves 2 `
  --seed 1592594996
```

Eight board symmetries are emitted for every source position. Use
`--no-augmentation` to emit one row per position and pass `--group-size 1`
to OthelloTrainer.
