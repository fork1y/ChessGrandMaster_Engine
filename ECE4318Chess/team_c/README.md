# Student Chess Engine (C) - UCI

This project builds a UCI-compatible chess engine in C for use with the course Arena program.

- Implements UCI over stdin/stdout
- Parses `position startpos ...` and `position fen ...`
- Generates legal moves, including castling and promotions
- Searches moves with a simple `negamax` search plus `alpha-beta pruning`
- Uses a lightweight evaluation based on material, piece-square bonuses, and mobility

## Build
`./build.sh`

Windows:
`build.bat`

## Run
`./run.sh`

Windows:
`run.bat`

## Arena Use
Build the engine first, then point Arena at the produced engine launcher in this folder.

- Ubuntu/Linux: use `run.sh` or the built `engine`
- Windows: use `run.bat` or the built `engine.exe`
