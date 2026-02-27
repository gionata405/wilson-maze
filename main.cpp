#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <algorithm>
#include <string>

// ─── Cell & helpers ───────────────────────────────────────────────────────────

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

// ─── Maze ─────────────────────────────────────────────────────────────────────

class Maze {
public:
    int width, height;
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
        std::uniform_int_distribution<int> dist(0, (int)unvisited.size() - 1);
        return unvisited[dist(rng)];
    }

    void generate() {
        Cell start = {width / 2, height / 2};
        visited[start] = true;
        int total = width * height;
        int visitedCount = 1;

        while (visitedCount < total) {
            Cell current = randomUnvisited();
            std::unordered_map<Cell, Direction, CellHash> path;
            std::vector<Cell> walkOrder;
            walkOrder.push_back(current);
            Cell walker = current;

            while (!visited[walker]) {
                for (Direction d : shuffledDirs()) {
                    Cell next = neighbor(walker, d);
                    if (inBounds(next)) {
                        path[walker] = d;
                        walker = next;
                        auto it = std::find(walkOrder.begin(), walkOrder.end(), walker);
                        if (it != walkOrder.end())
                            walkOrder.erase(it, walkOrder.end());
                        walkOrder.push_back(walker);
                        break;
                    }
                }
            }

            for (size_t i = 0; i + 1 < walkOrder.size(); i++) {
                Cell from = walkOrder[i];
                Cell to   = walkOrder[i + 1];
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

    bool hasPassage(Cell c, Direction d) const {
        auto it = passages.find(c);
        if (it == passages.end()) return false;
        for (Direction p : it->second)
            if (p == d) return true;
        return false;
    }
};

// ─── Constants ────────────────────────────────────────────────────────────────

const int MAZE_W   = 20;
const int MAZE_H   = 15;
const int CELL_SIZE = 40;
const int WALL_W   = 3;
const int MARGIN   = 40;

const sf::Color COLOR_BG       = sf::Color(20, 20, 30);
const sf::Color COLOR_WALL     = sf::Color(200, 200, 220);
const sf::Color COLOR_TRAIL    = sf::Color(220, 50, 50, 180);
const sf::Color COLOR_PLAYER   = sf::Color(80, 180, 255);
const sf::Color COLOR_ENTRANCE = sf::Color(80, 220, 120);
const sf::Color COLOR_EXIT     = sf::Color(255, 200, 60);

// ─── Drawing helpers ──────────────────────────────────────────────────────────

sf::Vector2f cellPos(int x, int y) {
    return { (float)(MARGIN + x * CELL_SIZE), (float)(MARGIN + y * CELL_SIZE) };
}

void drawMaze(sf::RenderWindow& win, const Maze& maze) {
    // Fill background
    sf::RectangleShape bg(sf::Vector2f(
        (float)(MAZE_W * CELL_SIZE + 2),
        (float)(MAZE_H * CELL_SIZE + 2)
    ));
    bg.setPosition(sf::Vector2f((float)(MARGIN - 1), (float)(MARGIN - 1)));
    bg.setFillColor(sf::Color(40, 40, 55));
    win.draw(bg);

    sf::RectangleShape wall;
    wall.setFillColor(COLOR_WALL);

    for (int y = 0; y < maze.height; y++) {
        for (int x = 0; x < maze.width; x++) {
            auto pos = cellPos(x, y);

            // North wall (skip if entrance opening at top-left)
            bool entranceNorth = (x == 0 && y == 0);
            if (!maze.hasPassage({x, y}, NORTH) && !entranceNorth) {
                wall.setSize(sf::Vector2f((float)(CELL_SIZE + WALL_W), (float)WALL_W));
                wall.setPosition(sf::Vector2f(pos.x - WALL_W / 2.f, pos.y - WALL_W / 2.f));
                win.draw(wall);
            }

            // West wall (skip entrance)
            if (!maze.hasPassage({x, y}, WEST)) {
                wall.setSize(sf::Vector2f((float)WALL_W, (float)(CELL_SIZE + WALL_W)));
                wall.setPosition(sf::Vector2f(pos.x - WALL_W / 2.f, pos.y - WALL_W / 2.f));
                win.draw(wall);
            }

            // South wall (bottom border)
            bool exitSouth = (x == maze.width - 1 && y == maze.height - 1);
            if (!maze.hasPassage({x, y}, SOUTH) && !exitSouth) {
                wall.setSize(sf::Vector2f((float)(CELL_SIZE + WALL_W), (float)WALL_W));
                wall.setPosition(sf::Vector2f(pos.x - WALL_W / 2.f, pos.y + CELL_SIZE - WALL_W / 2.f));
                win.draw(wall);
            }

            // East wall (right border)
            if (!maze.hasPassage({x, y}, EAST)) {
                wall.setSize(sf::Vector2f((float)WALL_W, (float)(CELL_SIZE + WALL_W)));
                wall.setPosition(sf::Vector2f(pos.x + CELL_SIZE - WALL_W / 2.f, pos.y - WALL_W / 2.f));
                win.draw(wall);
            }
        }
    }

    // Entrance marker (top-left, opening at top)
    sf::RectangleShape entrance(sf::Vector2f((float)CELL_SIZE - WALL_W * 2, 6.f));
    entrance.setFillColor(COLOR_ENTRANCE);
    entrance.setPosition(sf::Vector2f(cellPos(0, 0).x + WALL_W, cellPos(0, 0).y - 3.f));
    win.draw(entrance);

    // Exit marker (bottom-right, opening at bottom)
    sf::RectangleShape exit_(sf::Vector2f((float)CELL_SIZE - WALL_W * 2, 6.f));
    exit_.setFillColor(COLOR_EXIT);
    exit_.setPosition(sf::Vector2f(cellPos(MAZE_W - 1, MAZE_H - 1).x + WALL_W,
                      cellPos(MAZE_W - 1, MAZE_H - 1).y + CELL_SIZE - 3.f));
    win.draw(exit_);
}

void drawTrail(sf::RenderWindow& win, const std::vector<Cell>& trail) {
    if (trail.size() < 2) return;
    for (size_t i = 0; i + 1 < trail.size(); i++) {
        Cell a = trail[i], b = trail[i + 1];
        float cx = CELL_SIZE / 2.f;

        sf::RectangleShape seg;
        seg.setFillColor(COLOR_TRAIL);

        if (a.x == b.x) {
            // vertical
            int minY = std::min(a.y, b.y);
            seg.setSize(sf::Vector2f(6.f, (float)CELL_SIZE));
            seg.setPosition(sf::Vector2f(cellPos(a.x, minY).x + cx - 3.f, cellPos(a.x, minY).y + cx - 3.f));
        } else {
            // horizontal
            int minX = std::min(a.x, b.x);
            seg.setSize(sf::Vector2f((float)CELL_SIZE, 6.f));
            seg.setPosition(sf::Vector2f(cellPos(minX, a.y).x + cx - 3.f, cellPos(minX, a.y).y + cx - 3.f));
        }
        win.draw(seg);
    }
}

void drawPlayer(sf::RenderWindow& win, Cell player) {
    float cx = CELL_SIZE / 2.f;
    float r = CELL_SIZE * 0.3f;
    sf::CircleShape circle(r);
    circle.setFillColor(COLOR_PLAYER);
    circle.setOrigin(sf::Vector2f(r, r));
    auto pos = cellPos(player.x, player.y);
    circle.setPosition(sf::Vector2f(pos.x + cx, pos.y + cx));
    win.draw(circle);
}

void drawWinScreen(sf::RenderWindow& win, sf::Font& font) {
    sf::RectangleShape overlay(sf::Vector2f((float)win.getSize().x, (float)win.getSize().y));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    win.draw(overlay);

    sf::Text text(font, "You escaped the maze!", 36);
    text.setFillColor(sf::Color(255, 220, 80));
    text.setStyle(sf::Text::Bold);
    auto bounds = text.getLocalBounds();
    text.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
    text.setPosition(sf::Vector2f((float)win.getSize().x / 2.f, (float)win.getSize().y / 2.f - 20.f));
    win.draw(text);

    sf::Text sub(font, "Press R to play again or Esc to quit", 20);
    sub.setFillColor(sf::Color(200, 200, 200));
    auto sb = sub.getLocalBounds();
    sub.setOrigin(sf::Vector2f(sb.size.x / 2.f, sb.size.y / 2.f));
    sub.setPosition(sf::Vector2f((float)win.getSize().x / 2.f, (float)win.getSize().y / 2.f + 30.f));
    win.draw(sub);
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
    int winW = MAZE_W * CELL_SIZE + MARGIN * 2;
    int winH = MAZE_H * CELL_SIZE + MARGIN * 2;

    sf::RenderWindow window(
        sf::VideoMode({(unsigned int)winW, (unsigned int)winH}),
        "Wilson's Maze",
        sf::State::Windowed
    );
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")
                   || font.openFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf")
                   || font.openFromFile("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf");
    (void)fontLoaded;

    auto newGame = [&]() -> std::pair<Maze, std::vector<Cell>> {
        Maze maze(MAZE_W, MAZE_H);
        maze.generate();
        std::vector<Cell> trail;
        trail.push_back({0, 0}); // start at entrance
        return {std::move(maze), trail};
    };

    auto [maze, trail] = newGame();
    Cell exit_ = {MAZE_W - 1, MAZE_H - 1};
    bool won = false;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                    window.close();

                if (keyPressed->code == sf::Keyboard::Key::R) {
                    auto [m, t] = newGame();
                    maze  = std::move(m);
                    trail = std::move(t);
                    won   = false;
                }

                if (!won) {
                    Cell player = trail.back();
                    Direction dir;
                    bool moved = false;

                    if (keyPressed->code == sf::Keyboard::Key::W) { dir = NORTH; moved = true; }
                    if (keyPressed->code == sf::Keyboard::Key::S) { dir = SOUTH; moved = true; }
                    if (keyPressed->code == sf::Keyboard::Key::A) { dir = WEST;  moved = true; }
                    if (keyPressed->code == sf::Keyboard::Key::D) { dir = EAST;  moved = true; }

                    if (moved) {
                        Cell next = neighbor(player, dir);

                        bool isExit = (player == exit_ && dir == SOUTH);

                        if (isExit) {
                            won = true;
                        } else if (maze.inBounds(next) && maze.hasPassage(player, dir)) {
                            auto it = std::find(trail.begin(), trail.end(), next);
                            if (it != trail.end()) {
                                trail.erase(it + 1, trail.end());
                            } else {
                                trail.push_back(next);
                            }
                        }
                    }
                }
            }
        }

        window.clear(COLOR_BG);
        drawMaze(window, maze);
        drawTrail(window, trail);
        drawPlayer(window, trail.back());
        if (won) drawWinScreen(window, font);
        window.display();
    }

    return 0;
}
