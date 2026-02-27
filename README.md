# Wilson's Maze Generator

A C++ implementation of [Wilson's algorithm](https://en.wikipedia.org/wiki/Maze_generation_algorithm#Wilson's_algorithm) for generating uniform spanning tree mazes.

## How it works

Wilson's algorithm uses **loop-erased random walks** to generate a maze that is a uniform spanning tree — meaning every possible maze is equally likely to be generated.

1. Mark a random cell as visited
2. Pick any unvisited cell and start a random walk
3. If the walk revisits a cell, erase the loop
4. When the walk reaches a visited cell, carve the path into the maze
5. Repeat until all cells are visited

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
./wilson-maze          # default 20x10
./wilson-maze 40 20    # custom size
```

## Example output

```
+---+---+---+---+---+
|               |   |
+   +---+---+   +   +
|   |       |       |
+   +   +   +---+   +
|       |           |
+---+---+---+---+---+
```

## License

MIT
