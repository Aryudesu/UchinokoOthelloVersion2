# Neural move-ordering benchmark

This console tool compares heuristic move ordering with neural ordering on the
same positions. It can scan the neural minimum depth, legal-move threshold, and
neural/heuristic rank blend. It writes both raw results and an aggregate summary,
then prints conservative `game.ini` values when a neural condition wins.
The search itself caches neural outputs by `(board, turn)` during iterative
deepening; raw and summary CSVs report actual inference calls and cache hits.

## Build

Open `UchinokoOthelloVersion2.sln` in Visual Studio and build
`UchinokoOthelloBenchmark` in `Release | x64`.

## Quick automatic tuning

No existing search log is required. This creates ten deterministic mixed
self-play positions and evaluates a compact built-in parameter set:

```powershell
build\x64\Release\UchinokoOthelloBenchmark.exe `
  --generate-positions 10 `
  --model data\model\othello_ordering_20k.model `
  --output neural_ordering_benchmark.csv `
  --depth 8 `
  --auto-tune `
  --repeats 2 `
  --max-positions 10 `
  --time-limit-ms 3000
```

The raw output is `neural_ordering_benchmark.csv`. The aggregate output defaults
to `neural_ordering_benchmark_summary.csv`. A row with `recommended=1` is the
selected condition. The console also prints the corresponding `game.ini` keys.
It recommends `neural_ordering_enabled=false` when no neural condition clears
the conservative speed and completed-depth checks.
The built-in set covers root-only ordering through roughly the upper third of
the configured search, so a time-limited depth-15 search also tests minimum
depths below 15.

## Logged-position tuning

Real game logs are preferable for the final decision because they represent the
positions actually reached by the application. Run this from the repository
root in PowerShell:

```powershell
build\x64\Release\UchinokoOthelloBenchmark.exe `
  --input data\log\search_statistics1.csv `
  --input data\log\search_statistics2.csv `
  --model data\model\othello_ordering_20k.model `
  --output neural_ordering_benchmark.csv `
  --depth 12 `
  --auto-tune `
  --repeats 3 `
  --max-positions 20
```

For a manual grid, omit `--auto-tune` and repeat any of these options:

```text
--minimum-depth N
--minimum-legal-moves N
--blend-percent N
```

The grid is the Cartesian product of the supplied values. `--blend-percent=100`
uses neural order, while smaller values combine neural and heuristic ranks.
`--minimum-legal-moves` avoids paying inference cost at nodes where move ordering
has little pruning opportunity.

Input columns are found by header name, so additional search-statistics columns
do not affect loading. Rows recorded as opening-book moves and duplicate
`(black_bits, white_bits, turn)` positions are skipped. By default, only
positions with 17 through 43 empty squares are used. Pass
`--max-positions 0` to remove the position limit.

`--generate-positions` uses a reproducible mix of heuristic and random self-play.
Change it with `--seed`; the default seed is fixed so comparisons are repeatable.

Each search has its own deadline (`--time-limit-ms`, default 10000 ms). Condition
order is reversed in pairs and rotated between later pairs to reduce warm-up and
ordering bias. Use a Release build and keep the program active while collecting
data if you want the cleanest wall-clock comparison.

The summary calculates elapsed-time and node ratios only when both searches
finished without a timeout at the same depth. It separately counts conditions
that completed shallower or deeper than the baseline. This prevents a setting
from winning merely because it stopped early.
