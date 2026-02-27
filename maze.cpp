#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <algorithm>

struct Cell {
    int x, y;
    bool operator==(const Cell& o) const { return x == o.x && y == o.y; }
};

struct CellHash {
    size_t operator()(const Cell& c) const {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.y) << 16);
    }
};

enum Direction { NORTH, SOUTH, EAST, WEST };

Cell neighbor(Cell c, Direction d) {
    switch (d) {
        case NORTH: return {c.x, c.y - 1};
        case SOUTH: return {c.x, c.y + 1};
        case EAST:  return {c.x + 1, c.y};
        case WEST:  return {c.x - 1, c.y};
    }
    return c;
}

Direction opposite(Direction d) {
    switch (d) {
        case NORTH: return SOUTH;
        case SOUTH: return NORTH;
        case EAST:  return WEST;
        case WEST:  return EAST;
    }
    return d;
}

class Maze {
public:
    int width, height;

    // passages[cell] = set of directions with open walls
    std::unordered_map<Cell, std::vector<Direction>, CellHash> passages;
    std::unordered_map<Cell, bool, CellHash> visited;

    std::mt19937 rng;

    Maze(int w, int h) : width(w), height(h) {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng.seed(seed);
    }

    bool inBounds(Cell c) {
        return c.x >= 0 && c.x < width && c.y >= 0 && c.y < height;
    }

    std::vector<Direction> shuffledDirs() {
        std::vector<Direction> dirs = {NORTH, SOUTH, EAST, WEST};
        std::shuffle(dirs.begin(), dirs.end(), rng);
        return dirs;
    }

    Cell randomUnvisited() {
        std::vector<Cell> unvisited;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                if (!visited[{x, y}])
                    unvisited.push_back({x, y});
        std::uniform_int_distribution<int> dist(0, unvisited.size() - 1);
        return unvisited[dist(rng)];
    }

    // Wilson's algorithm: loop-erased random walk
    void generate() {
        // Start by marking a random cell as visited
        Cell start = {width / 2, height / 2};
        visited[start] = true;

        int total = width * height;
        int visitedCount = 1;

        while (visitedCount < total) {
            // Pick a random unvisited cell to start a walk from
            Cell current = randomUnvisited();

            // Random walk — track the path and direction taken
            std::unordered_map<Cell, Direction, CellHash> path;
            std::vector<Cell> walkOrder;
            walkOrder.push_back(current);

            Cell walker = current;

            while (!visited[walker]) {
                auto dirs = shuffledDirs();
                for (Direction d : dirs) {
                    Cell next = neighbor(walker, d);
                    if (inBounds(next)) {
                        path[walker] = d; // direction taken from walker
                        walker = next;
                        // Loop erasure: if we've visited walker before in this walk, erase the loop
                        auto it = std::find(walkOrder.begin(), walkOrder.end(), walker);
                        if (it != walkOrder.end()) {
                            walkOrder.erase(it, walkOrder.end());
                        }
                        walkOrder.push_back(walker);
                        break;
                    }
                }
            }

            // Carve the path into the maze
            for (size_t i = 0; i + 1 < walkOrder.size(); i++) {
                Cell from = walkOrder[i];
                Cell to   = walkOrder[i + 1];

                // Find direction from -> to
                Direction d;
                if      (to.x == from.x && to.y == from.y - 1) d = NORTH;
                else if (to.x == from.x && to.y == from.y + 1) d = SOUTH;
                else if (to.x == from.x + 1 && to.y == from.y) d = EAST;
                else                                             d = WEST;

                passages[from].push_back(d);
                passages[to].push_back(opposite(d));
                visited[from] = true;
                visitedCount++;
            }
            visited[walker] = true;
        }
    }

    bool hasPassage(Cell c, Direction d) {
        auto it = passages.find(c);
        if (it == passages.end()) return false;
        for (Direction p : it->second)
            if (p == d) return true;
        return false;
    }

    void print() {
        // Top border
        std::cout << "+";
        for (int x = 0; x < width; x++) std::cout << "---+";
        std::cout << "\n";

        for (int y = 0; y < height; y++) {
            // Cell row
            std::cout << "|";
            for (int x = 0; x < width; x++) {
                std::cout << "   ";
                if (hasPassage({x, y}, EAST)) std::cout << " ";
                else                           std::cout << "|";
            }
            std::cout << "\n";

            // Bottom wall row
            std::cout << "+";
            for (int x = 0; x < width; x++) {
                if (hasPassage({x, y}, SOUTH)) std::cout << "   +";
                else                            std::cout << "---+";
            }
            std::cout << "\n";
        }
    }
};

int main(int argc, char* argv[]) {
    int width  = (argc > 1) ? std::atoi(argv[1]) : 20;
    int height = (argc > 2) ? std::atoi(argv[2]) : 10;

    if (width < 2 || height < 2 || width > 100 || height > 100) {
        std::cerr << "Usage: " << argv[0] << " [width] [height]\n";
        std::cerr << "  width and height must be between 2 and 100 (default: 20x10)\n";
        return 1;
    }

    std::cout << "Wilson's Maze Generator — " << width << "x" << height << "\n\n";

    Maze maze(width, height);
    maze.generate();
    maze.print();

    return 0;
}
