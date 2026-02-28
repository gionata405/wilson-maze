#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <iostream>

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

// ─── Config ────────────────────────────────────────────────────────────────────

struct Config {
    int mazeW   = 20;
    int mazeH   = 15;
    int margin  = 40;
    int wallW   = 3;

    // Derived — recomputed on resize
    int cellSize = 40;

    void recomputeCellSize(int winW, int winH) {
        int availW = winW - margin * 2;
        int availH = winH - margin * 2;
        cellSize = std::max(10, std::min(availW / mazeW, availH / mazeH));
    }

    int windowW() const { return mazeW * cellSize + margin * 2; }
    int windowH() const { return mazeH * cellSize + margin * 2; }
};

// ─── Colors ────────────────────────────────────────────────────────────────────

const sf::Color COLOR_BG       = sf::Color(20, 20, 30);
const sf::Color COLOR_WALL     = sf::Color(200, 200, 220);
const sf::Color COLOR_TRAIL    = sf::Color(220, 50, 50, 180);
const sf::Color COLOR_PLAYER   = sf::Color(80, 180, 255);
const sf::Color COLOR_ENTRANCE = sf::Color(80, 220, 120);
const sf::Color COLOR_EXIT     = sf::Color(255, 200, 60);

// ─── Drawing helpers ──────────────────────────────────────────────────────────

sf::Vector2f cellPos(int x, int y, const Config& cfg) {
    return { (float)(cfg.margin + x * cfg.cellSize), (float)(cfg.margin + y * cfg.cellSize) };
}

void drawMaze(sf::RenderWindow& win, const Maze& maze, const Config& cfg) {
    sf::RectangleShape bg(sf::Vector2f(
        (float)(cfg.mazeW * cfg.cellSize + 2),
        (float)(cfg.mazeH * cfg.cellSize + 2)
    ));
    bg.setPosition(sf::Vector2f((float)(cfg.margin - 1), (float)(cfg.margin - 1)));
    bg.setFillColor(sf::Color(40, 40, 55));
    win.draw(bg);

    sf::RectangleShape wall;
    wall.setFillColor(COLOR_WALL);

    for (int y = 0; y < maze.height; y++) {
        for (int x = 0; x < maze.width; x++) {
            auto pos = cellPos(x, y, cfg);
            float wf = (float)cfg.wallW;
            float cs = (float)cfg.cellSize;

            bool entranceNorth = (x == 0 && y == 0);
            if (!maze.hasPassage({x, y}, NORTH) && !entranceNorth) {
                wall.setSize(sf::Vector2f(cs + wf, wf));
                wall.setPosition(sf::Vector2f(pos.x - wf/2.f, pos.y - wf/2.f));
                win.draw(wall);
            }
            if (!maze.hasPassage({x, y}, WEST)) {
                wall.setSize(sf::Vector2f(wf, cs + wf));
                wall.setPosition(sf::Vector2f(pos.x - wf/2.f, pos.y - wf/2.f));
                win.draw(wall);
            }
            bool exitSouth = (x == maze.width - 1 && y == maze.height - 1);
            if (!maze.hasPassage({x, y}, SOUTH) && !exitSouth) {
                wall.setSize(sf::Vector2f(cs + wf, wf));
                wall.setPosition(sf::Vector2f(pos.x - wf/2.f, pos.y + cs - wf/2.f));
                win.draw(wall);
            }
            if (!maze.hasPassage({x, y}, EAST)) {
                wall.setSize(sf::Vector2f(wf, cs + wf));
                wall.setPosition(sf::Vector2f(pos.x + cs - wf/2.f, pos.y - wf/2.f));
                win.draw(wall);
            }
        }
    }

    // Entrance / exit markers
    float wf = (float)cfg.wallW;
    float cs = (float)cfg.cellSize;

    sf::RectangleShape entrance(sf::Vector2f(cs - wf * 2, 6.f));
    entrance.setFillColor(COLOR_ENTRANCE);
    entrance.setPosition(sf::Vector2f(cellPos(0, 0, cfg).x + wf, cellPos(0, 0, cfg).y - 3.f));
    win.draw(entrance);

    sf::RectangleShape exitMark(sf::Vector2f(cs - wf * 2, 6.f));
    exitMark.setFillColor(COLOR_EXIT);
    auto ep = cellPos(cfg.mazeW - 1, cfg.mazeH - 1, cfg);
    exitMark.setPosition(sf::Vector2f(ep.x + wf, ep.y + cs - 3.f));
    win.draw(exitMark);
}

void drawTrail(sf::RenderWindow& win, const std::vector<Cell>& trail, const Config& cfg) {
    if (trail.size() < 2) return;
    float cx = cfg.cellSize / 2.f;
    for (size_t i = 0; i + 1 < trail.size(); i++) {
        Cell a = trail[i], b = trail[i + 1];
        sf::RectangleShape seg;
        seg.setFillColor(COLOR_TRAIL);
        if (a.x == b.x) {
            int minY = std::min(a.y, b.y);
            seg.setSize(sf::Vector2f(6.f, (float)cfg.cellSize));
            seg.setPosition(sf::Vector2f(cellPos(a.x, minY, cfg).x + cx - 3.f,
                                         cellPos(a.x, minY, cfg).y + cx - 3.f));
        } else {
            int minX = std::min(a.x, b.x);
            seg.setSize(sf::Vector2f((float)cfg.cellSize, 6.f));
            seg.setPosition(sf::Vector2f(cellPos(minX, a.y, cfg).x + cx - 3.f,
                                         cellPos(minX, a.y, cfg).y + cx - 3.f));
        }
        win.draw(seg);
    }
}

void drawPlayer(sf::RenderWindow& win, Cell player, const Config& cfg) {
    float cx = cfg.cellSize / 2.f;
    float r  = cfg.cellSize * 0.3f;
    sf::CircleShape circle(r);
    circle.setFillColor(COLOR_PLAYER);
    circle.setOrigin(sf::Vector2f(r, r));
    auto pos = cellPos(player.x, player.y, cfg);
    circle.setPosition(sf::Vector2f(pos.x + cx, pos.y + cx));
    win.draw(circle);
}

void drawHUD(sf::RenderWindow& win, sf::Font& font, const Config& cfg) {
    std::string info = std::to_string(cfg.mazeW) + "x" + std::to_string(cfg.mazeH)
                     + "  WASD: move  R: new maze  +/-: resize";
    sf::Text hud(font, info, 12);
    hud.setFillColor(sf::Color(100, 100, 120));
    hud.setPosition(sf::Vector2f(10.f, 4.f));
    win.draw(hud);
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

int main(int argc, char* argv[]) {
    Config cfg;

    // Parse optional args: wilson-maze [width] [height]
    if (argc >= 2) cfg.mazeW = std::max(5, std::atoi(argv[1]));
    if (argc >= 3) cfg.mazeH = std::max(5, std::atoi(argv[2]));

    sf::RenderWindow window(
        sf::VideoMode({(unsigned int)cfg.windowW(), (unsigned int)cfg.windowH()}),
        "Wilson's Maze  " + std::to_string(cfg.mazeW) + "x" + std::to_string(cfg.mazeH),
        sf::State::Windowed
    );
    window.setFramerateLimit(60);

    sf::Font font;
    bool fontLoaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")
                   || font.openFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf")
                   || font.openFromFile("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf");
    (void)fontLoaded;

    auto newGame = [&]() -> std::pair<Maze, std::vector<Cell>> {
        Maze maze(cfg.mazeW, cfg.mazeH);
        maze.generate();
        std::vector<Cell> trail;
        trail.push_back({0, 0});
        return {std::move(maze), trail};
    };

    auto resizeWindow = [&]() {
        window.setSize({(unsigned int)cfg.windowW(), (unsigned int)cfg.windowH()});
        window.setTitle("Wilson's Maze  " + std::to_string(cfg.mazeW) + "x" + std::to_string(cfg.mazeH));
    };

    auto [maze, trail] = newGame();
    Cell exitCell = {cfg.mazeW - 1, cfg.mazeH - 1};
    bool won = false;

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            // Handle resize
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::FloatRect view(sf::Vector2f(0.f, 0.f),
                                   sf::Vector2f((float)resized->size.x, (float)resized->size.y));
                window.setView(sf::View(view));
                cfg.recomputeCellSize((int)resized->size.x, (int)resized->size.y);
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                    window.close();

                // R: new maze (same size)
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    auto [m, t] = newGame();
                    maze  = std::move(m);
                    trail = std::move(t);
                    exitCell = {cfg.mazeW - 1, cfg.mazeH - 1};
                    won = false;
                }

                // + / =: increase maze size
                if (keyPressed->code == sf::Keyboard::Key::Equal ||
                    keyPressed->code == sf::Keyboard::Key::Add) {
                    cfg.mazeW = std::min(cfg.mazeW + 2, 60);
                    cfg.mazeH = std::min(cfg.mazeH + 2, 45);
                    resizeWindow();
                    auto [m, t] = newGame();
                    maze = std::move(m); trail = std::move(t);
                    exitCell = {cfg.mazeW - 1, cfg.mazeH - 1};
                    won = false;
                }

                // - : decrease maze size
                if (keyPressed->code == sf::Keyboard::Key::Hyphen ||
                    keyPressed->code == sf::Keyboard::Key::Subtract) {
                    cfg.mazeW = std::max(cfg.mazeW - 2, 5);
                    cfg.mazeH = std::max(cfg.mazeH - 2, 5);
                    resizeWindow();
                    auto [m, t] = newGame();
                    maze = std::move(m); trail = std::move(t);
                    exitCell = {cfg.mazeW - 1, cfg.mazeH - 1};
                    won = false;
                }

                if (!won) {
                    Cell player = trail.back();
                    Direction dir;
                    bool moved = false;

                    if (keyPressed->code == sf::Keyboard::Key::W || keyPressed->code == sf::Keyboard::Key::Up)
                        { dir = NORTH; moved = true; }
                    if (keyPressed->code == sf::Keyboard::Key::S || keyPressed->code == sf::Keyboard::Key::Down)
                        { dir = SOUTH; moved = true; }
                    if (keyPressed->code == sf::Keyboard::Key::A || keyPressed->code == sf::Keyboard::Key::Left)
                        { dir = WEST;  moved = true; }
                    if (keyPressed->code == sf::Keyboard::Key::D || keyPressed->code == sf::Keyboard::Key::Right)
                        { dir = EAST;  moved = true; }

                    if (moved) {
                        Cell next = neighbor(player, dir);
                        bool isExit = (player == exitCell && dir == SOUTH);

                        if (isExit) {
                            won = true;
                        } else if (maze.inBounds(next) && maze.hasPassage(player, dir)) {
                            auto it = std::find(trail.begin(), trail.end(), next);
                            if (it != trail.end())
                                trail.erase(it + 1, trail.end());
                            else
                                trail.push_back(next);
                        }
                    }
                }
            }
        }

        window.clear(COLOR_BG);
        drawMaze(window, maze, cfg);
        drawTrail(window, trail, cfg);
        drawPlayer(window, trail.back(), cfg);
        drawHUD(window, font, cfg);
        if (won) drawWinScreen(window, font);
        window.display();
    }

    return 0;
}
