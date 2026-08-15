#include <game/game.hpp>
#include <debug/logs.hpp>
#include <fstream>

int main() {
    Log::Level::set(Logging::Info);
    // pickup map name from assets/map/default_map.txt
    std::string initialMap;
    std::ifstream mapFile("assets/maps/default_map.txt");
    if (mapFile.is_open()) {
        std::getline(mapFile, initialMap);
        mapFile.close();
    } else {
        Log::warn << "Could not open default_map.txt, using 'default' as initial map.\n";
        initialMap = "default";
    }
    Game::Game game("Hello, SFML!", 1200, 600, initialMap);
    game.run();
    return 0;
}