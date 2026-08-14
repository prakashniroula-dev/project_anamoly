#include <game/game.hpp>
#include <debug/logs.hpp>

int main() {
    Log::Level::set(Logging::Info);
    Game::Game game("Hello, SFML!", 1200, 600);
    game.run();
    return 0;
}