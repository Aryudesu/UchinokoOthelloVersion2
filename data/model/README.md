# Neural ordering model

Place the trained model named by `neural_model_path` in this directory, then set
`neural_ordering_enabled=true` in `data/config/game.ini`. The default Hard
profile expects `othello_ordering_20k.model`.

Neural move ordering can be tuned with:

- `neural_ordering_minimum_depth`
- `neural_ordering_minimum_legal_moves`
- `neural_ordering_blend_percent`

Use `UchinokoOthelloBenchmark --auto-tune` to compare conservative candidates
for the current model and search depth.

Model files are generated locally and are not committed to Git.
