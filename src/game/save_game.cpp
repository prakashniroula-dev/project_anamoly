#include "save_game.hpp"
#include <fstream>
#include <sstream>
#include <debug/logs.hpp>

bool SaveGame::save(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        Log::error << "Failed to open save file for writing: " << filepath << "\n";
        return false;
    }
    out << "map=" << mapName << "\n";
    out << "playerX=" << playerPos.x << "\n";
    out << "playerY=" << playerPos.y << "\n";
    out << "playerChar=" << playerCharacter << "\n";
    // Write story state using the same format as StoryManager
    out << "#flags\n";
    for (const auto& [k, v] : flags)
        out << k << "=" << (v ? "1" : "0") << "\n";
    out << "#items\n";
    for (const auto& [k, v] : items)
        out << k << "=" << (v ? "1" : "0") << "\n";
    out << "#choices\n";
    for (const auto& choice : choicesMade)
        out << choice << "\n";
    out << "#npcs\n";
    for (const auto& [id, state] : npcStates) {
        out << id << " " << (state.autoTalked ? 1 : 0) << " " << (state.talked ? 1 : 0) << "\n";
    }
    out << "#end\n";
    return true;
}

bool SaveGame::load(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        Log::error << "Failed to open save file for reading: " << filepath << "\n";
        return false;
    }
    std::string line, section;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') {
            section = line.substr(1);
            continue;
        }
        // Parse key=value lines (only before the story sections)
        if (section.empty()) {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq+1);
            if (key == "map") mapName = val;
            else if (key == "playerX") playerPos.x = std::stof(val);
            else if (key == "playerY") playerPos.y = std::stof(val);
            else if (key == "playerChar") playerCharacter = val;
        } else {
            // Section parsing for flags/items/choices
            if (section == "flags") {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string k = line.substr(0, eq);
                    bool v = (line.substr(eq+1) == "1");
                    flags[k] = v;
                }
            } else if (section == "items") {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string k = line.substr(0, eq);
                    bool v = (line.substr(eq+1) == "1");
                    items[k] = v;
                }
            } else if (section == "choices") {
                choicesMade.push_back(line);
            } else if (section == "npcs") {
                std::istringstream iss(line);
                std::string id;
                int autoTalked, talked;
                if (iss >> id >> autoTalked >> talked) {
                    NPCState state;
                    state.autoTalked = (autoTalked != 0);
                    state.talked = (talked != 0);
                    npcStates[id] = state;
                }
            } else if (section == "end") {
                break;
            }
        }
    }
    return true;
}