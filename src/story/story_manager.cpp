#include "story_manager.hpp"
#include <fstream>

void StoryManager::clearAll() {
    flags.clear();
    items.clear();
    choicesMade.clear();
}

void StoryManager::saveToStream(std::ostream& out) const {
    out << "#flags\n";
    for (const auto& [k, v] : flags)
        out << k << "=" << (v ? "1" : "0") << "\n";
    out << "#items\n";
    for (const auto& [k, v] : items)
        out << k << "=" << (v ? "1" : "0") << "\n";
    out << "#choices\n";
    for (const auto& choice : choicesMade)
        out << choice << "\n";
    out << "#end\n";
}

void StoryManager::loadFromStream(std::istream& in) {
    flags.clear();
    items.clear();
    choicesMade.clear();
    std::string line, section;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') {
            section = line.substr(1);
            continue;
        }
        if (section == "flags") {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                bool val = (line.substr(eq+1) == "1");
                flags[key] = val;
            }
        } else if (section == "items") {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                bool val = (line.substr(eq+1) == "1");
                items[key] = val;
            }
        } else if (section == "choices") {
            choicesMade.push_back(line);
        } else if (section == "end") {
            break;
        }
    }
}