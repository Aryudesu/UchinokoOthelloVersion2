# Neural move-ordering benchmark

This console tool replays the same logged positions with heuristic ordering and
with one or more neural-ordering minimum depths. It writes one raw CSV row for
each condition, position, and repeat, including elapsed-time and node ratios
relative to the heuristic baseline.

## Build

Open `UchinokoOthelloVersion2.sln` in Visual Studio and build
`UchinokoOthelloBenchmark` in `Release | x64`.

## Example

Run this from the repository root in PowerShell:

```powershell
build\x64\Release\UchinokoOthelloBenchmark.exe `
  --input data\log\search_statistics1.csv `
  --input data\log\search_statistics2.csv `
  --model data\model\othello_ordering_20k.model `
  --output neural_ordering_benchmark.csv `
  --depth 12 `
  --minimum-depth 10 `
  --minimum-depth 11 `
  --minimum-depth 12 `
  --repeats 3 `
  --max-positions 20
```

Input columns are found by header name, so additional search-statistics columns
do not affect loading. Rows recorded as opening-book moves and duplicate
`(black_bits, white_bits, turn)` positions are skipped. By default, only
positions with 17 through 43 empty squares are used. Pass
`--max-positions 0` to remove the position limit.

Each search has its own deadline (`--time-limit-ms`, default 10000 ms). On every
second repeat the condition order is reversed to reduce warm-up and ordering
bias. Use a Release build and keep the program active while collecting data if
you want the cleanest wall-clock comparison.
