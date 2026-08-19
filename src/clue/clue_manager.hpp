// clue/clue_manager.hpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

enum class ParagraphType {
    Plain,
    Bullet,
    Underline,
    Highlight
};

struct ClueParagraph {
    ParagraphType type;
    std::string text;
};

struct ClueInfo {
    std::string id;
    std::string title;
    std::string evaluateState;
    std::string executeState;
    std::vector<ClueParagraph> paragraphs;   // instead of a single description
    std::string clue_type;

    ClueInfo() : clue_type("clue") {}

    ClueInfo& condition(std::string evalState) {
        evaluateState = evalState;
        return *this;
    }

    ClueInfo& action(std::string execState) {
        executeState = execState;
        return *this;
    }

    ClueInfo& addPlain(const std::string& text) {
        paragraphs.push_back({ParagraphType::Plain, text});
        return *this;
    }

    ClueInfo& type(const std::string& typ) {
      clue_type = typ;
      return *this;
    }

    ClueInfo& addBullet(const std::string& text) {
        paragraphs.push_back({ParagraphType::Bullet, text});
        return *this;
    }

    ClueInfo& addHighlight(const std::string& text) {
        paragraphs.push_back({ParagraphType::Highlight, text});
        return *this;
    }
};

class ClueManager {
public:
    static ClueManager& get();

    // Register a clue definition (call once at startup)
    void addClue(const ClueInfo& info);

    void addClues(const std::vector<ClueInfo>& clues) {
        for (const auto& clue : clues) {
            addClue(clue);
        }
    }

    bool isInspectable(std::string id);

    // Discover a clue (mark as found)
    void discoverClue(const std::string& id, bool value = true);

    // Check if a clue has been discovered
    bool isDiscovered(const std::string& id) const;


    void loadDiscovered(const std::vector<std::string>& discovered) {
      clearAll(); // Clear previous state before loading
      for (auto& id : discovered) {
        discoverClue(id);
      }
    }

    // Get all discovered clue IDs (in discovery order)
    const std::vector<std::string>& getDiscoveredClues() const;

    // Get clue info by ID (returns nullptr if not found)
    const ClueInfo* getClueInfo(const std::string& id) const;

    // Clear all state (for new game)
    void clearAll();

    // Save/load for persistence
    void saveToStream(std::ostream& out) const;
    void loadFromStream(std::istream& in);

    static void registerClues();

private:
    ClueManager() = default;

    std::unordered_map<std::string, ClueInfo> m_definitions;
    std::unordered_set<std::string> m_discoveredSet;
    std::vector<std::string> m_discoveredOrder; // maintains discovery order
};