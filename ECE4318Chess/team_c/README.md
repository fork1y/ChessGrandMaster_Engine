# Student Chess Engine (C) - UCI

This project builds a UCI-compatible chess engine in C for use with the course Arena program.

- Implements UCI over stdin/stdout
- Parses `position startpos ...` and `position fen ...`
- Generates legal moves, including castling and promotions
- Searches moves with a simple `negamax` search plus `alpha-beta pruning`
- Uses a lightweight evaluation based on material, piece-square bonuses, and mobility

## Build
`./build.sh`

## Run
`./run.sh`

## Arena Use
Build the engine first, then point Arena at the produced engine executable in this folder.
