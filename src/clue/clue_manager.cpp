// clue/clue_manager.cpp
#include "clue_manager.hpp"
#include <debug/logs.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <story/story_helpers.hpp>
#include <ui/notification.hpp>


ClueManager& ClueManager::get() {
    static ClueManager instance;
    return instance;
}

void ClueManager::addClue(const ClueInfo& info) {
    m_definitions[info.id] = info;
    Log::info << "Clue added: " << info.id << " - " << info.title << "\n";
}

bool ClueManager::isInspectable(std::string id) {
    ClueInfo& clue = m_definitions[id];
    Log::info << "Checking if clue is inspectable: " << id << " - " << clue.title << "\n";
    Log::info << "Result: " << StoryHelpers::evaluateCondition(clue.evaluateState) << "\n";
    return StoryHelpers::evaluateCondition(clue.evaluateState);
}

void ClueManager::discoverClue(const std::string& id, bool value) {
  ClueInfo& clue = m_definitions[id];
  
  if (m_discoveredSet.find(id) != m_discoveredSet.end())
  return; // already discovered
  
  if (m_definitions.find(id) == m_definitions.end()) {
    Log::warn << "Attempted to discover unknown clue: " << id << "\n";
    return;
  }
  
  if (!value) {
    // If value is false, we are undiscovering the clue
    m_discoveredSet.erase(id);
    m_discoveredOrder.erase(std::remove(m_discoveredOrder.begin(), m_discoveredOrder.end(), id), m_discoveredOrder.end());
    if (clue.clue_type == "item") {
      StoryHelpers::executeAction("takeItem(" + id + ")");
    }
    Log::info << "Clue undiscovered: " << id << "\n";
    return;
  }
  m_discoveredSet.insert(id);
  m_discoveredOrder.push_back(id);
  if ( clue.clue_type == "item") {
    StoryHelpers::executeAction("giveItem(" + id + ")");
  }
  StoryHelpers::executeAction(clue.executeState);
  
  if (StoryHelpers::evaluateCondition("!hasFlag(first_clue)")) {
    Notification::show("Tip", "Presssing `J` will open journal, with all clues you've collected.");
    StoryHelpers::executeAction("setFlag(first_clue)");
  }
  Log::info << "Clue discovered: " << id << "\n";
}

bool ClueManager::isDiscovered(const std::string& id) const {
    return m_discoveredSet.find(id) != m_discoveredSet.end();
}

const std::vector<std::string>& ClueManager::getDiscoveredClues() const {
    return m_discoveredOrder;
}

const ClueInfo* ClueManager::getClueInfo(const std::string& id) const {
    auto it = m_definitions.find(id);
    if (it != m_definitions.end())
        return &it->second;
    return nullptr;
}

void ClueManager::clearAll() {
    m_discoveredSet.clear();
    m_discoveredOrder.clear();
}

void ClueManager::saveToStream(std::ostream& out) const {
    for (const auto& id : m_discoveredOrder) {
        out << id << "\n";
    }
    out << "#endclues\n";
}

void ClueManager::loadFromStream(std::istream& in) {
    clearAll();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line == "#endclues") break;
        if (line[0] == '#') continue;        // skip any extra headers
        discoverClue(line);                  // restore discovered clue
    }
}