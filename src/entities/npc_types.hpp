#pragma once
#include <string>
#include <vector>
#include <SFML/System/Vector2.hpp>

struct DialogLine {
    std::string id;
    std::string speaker;
    std::string text;
    std::string condition;
    std::string action;
    int nextIndex = -1;
    std::vector<DialogLine> options;
    std::string soundKey;   // new: e.g., "npc_talk_angry"

    DialogLine(const std::string& id = ""): id(id) {}

    inline DialogLine& exchange(const std::string& speaker, const std::string& text) {
        this->speaker = speaker;
        this->text = text;
        return *this;
    }
    inline DialogLine& setCondition(const std::string& cond) {
        this->condition = cond;
        return *this;
    }
    inline DialogLine& setAction(const std::string& act) {
        this->action = act;
        return *this;
    }
    inline DialogLine& addOption(const DialogLine& option) {
        options.push_back(option);
        return *this;
    }
    inline DialogLine& sound(const std::string& soundKey) {
        this->soundKey = soundKey;
        return *this;
    }
    inline DialogLine& setOptions(const std::vector<DialogLine>& opts) {
        options = opts;
        return *this;
    }
    inline DialogLine& next(int nextIndex) {
        this->nextIndex = nextIndex;
        return *this;
    }
};

struct NPCType {
    std::string id;                  // unique type key, e.g. "detective_explainer"
    std::string characterKey;        // sprite key from Characters
    std::string behaviorType;        // "idle", "patrol", "follow", "scripted"
    std::vector<DialogLine> dialogue;
    std::string scriptName;          // optional, for special logic (e.g., "detective")
    std::vector<sf::Vector2f> waypoints;
    float talkRadius = 150.f;
    bool autoStartDialogue = false;
    float autoStartDelay = 0.f;
    std::string cutsceneScriptName;  // if non‑empty, this NPC triggers a cutscene
    float cutsceneRadius = 150.f;
    bool cutsceneOnce = true;
};