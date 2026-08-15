#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

class StoryManager {
public:
    static StoryManager& get() {
        static StoryManager instance;
        return instance;
    }

    void clearAll();   // reset for New Game
    void saveToStream(std::ostream& out) const;
    void loadFromStream(std::istream& in);

    // Expose internal data for SaveGame copying (or make SaveGame a friend)
    const auto& getFlags() const { return flags; }
    const auto& getItems() const { return items; }
    const auto& getChoices() const { return choicesMade; }
    void setFlags(const std::unordered_map<std::string, bool>& f) { flags = f; }
    void setItems(const std::unordered_map<std::string, bool>& i) { items = i; }
    void setChoices(const std::vector<std::string>& c) { choicesMade = c; }

    // Flags
    void setFlag(const std::string& key, bool value = true) { flags[key] = value; }
    bool hasFlag(const std::string& key) const { return flags.find(key) != flags.end() && flags.at(key); }
    bool getFlag(const std::string& key, bool def = false) const {
        auto it = flags.find(key);
        return (it != flags.end()) ? it->second : def;
    }

    // Items (simple boolean presence)
    void giveItem(const std::string& item) { items[item] = true; }
    bool hasItem(const std::string& item) const { return items.find(item) != items.end() && items.at(item); }

    // Choices made (for later reference)
    void addChoice(const std::string& choiceId) { choicesMade.push_back(choiceId); }
    const std::vector<std::string>& getChoicesMade() const { return choicesMade; }
    bool wasChoiceMade(const std::string& choiceId) const {
        return std::find(choicesMade.begin(), choicesMade.end(), choiceId) != choicesMade.end();
    }

private:
    std::unordered_map<std::string, bool> flags;
    std::unordered_map<std::string, bool> items;
    std::vector<std::string> choicesMade;
};