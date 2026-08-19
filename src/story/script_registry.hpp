// ScriptRegistry.hpp
#pragma once

#include <vector>
#include <unordered_map>
#include <vector>
#include <unordered_map>
#include <string>
#include <SFML/System/Vector2.hpp>
#include <functional>
#include <unordered_map>
#include <variant>

class NPC; // forward

enum class ActionType {
    MoveTo,
    MoveRelative,
    MoveTowardsPlayer,
    Wait,
    LockPlayer,
    UnlockPlayer,
    PlayAnimation,
    ShowDialogue,
    SwapPlayer,
    FacePlayer,
    CallFunction,
    EvaluateState,
    ExecuteState,
    Truthful,
    Falseful,
    EndSequence
};

namespace Action {
    struct Action {
        ActionType type;
        std::variant<
            float,
            int,
            bool,
            sf::Vector2f,
            std::string,
            NPC*
        > param;
        std::variant<
            float,
            int,
            bool,
            sf::Vector2f,
            std::string,
            NPC*
        > param2;
        Action(ActionType type = ActionType::EndSequence) : type(type), param(0), param2(0) {}
    };

    inline Action MoveTo(float x, float y) {
        Action a(ActionType::MoveTo);
        a.param = sf::Vector2f(x, y);
        return a;
    }
    
    inline Action MoveRelative(float dx, float dy) {
        Action a(ActionType::MoveRelative);
        a.param = sf::Vector2f(dx, dy);
        return a;
    }
    
    inline Action Wait(float seconds) {
        Action a(ActionType::Wait);
        a.param = seconds;
        return a;
    }
    
    inline Action LockPlayer() {
        return Action(ActionType::LockPlayer);
    }

    inline Action UnlockPlayer() {
        return Action(ActionType::UnlockPlayer);
    }
    
    inline Action PlayAnimation(const std::string& animKey) {
        Action a(ActionType::PlayAnimation);
        a.param = animKey;
        return a;
    }
    
    inline Action ShowDialogue(const std::string& dialogueId, bool allowEscape = true) {
        Action a(ActionType::ShowDialogue);
        a.param = dialogueId;
        a.param2 = allowEscape;
        return a;
    }
    
    inline Action SwapPlayer(NPC* npc) {
        Action a(ActionType::SwapPlayer);
        a.param = npc;
        return a;
    }
    
    inline Action FacePlayer() {
        return Action(ActionType::FacePlayer);
    }
    
    inline Action CallFunction(const std::string& functionName) {
        Action a(ActionType::CallFunction);
        a.param = functionName;
        return a;
    }
    
    inline Action EndSequence() {
        return Action(ActionType::EndSequence);
    }

    inline Action MoveTowardsPlayer(float amt = -1.f) {
        Action a = ActionType::MoveTowardsPlayer;
        a.param = amt;
        return a;
    }

    inline Action EvaluateState(const std::string& evalName) {
        Action a = ActionType::EvaluateState;
        a.param = evalName;
        return a;
    }

    inline Action ExecuteState(const std::string& execName) {
        Action a = ActionType::ExecuteState;
        a.param = execName;
        return a;
    }

    inline Action Truthful() {
        return Action(ActionType::Truthful);
    }

    inline Action Falseful() {
        return Action(ActionType::Falseful);
    }
}


using Script = std::vector<Action::Action>;
namespace ScriptRegistry {
  extern std::unordered_map<std::string, Script> scripts;
  void init();
}

namespace FunctionRegistry {
    using Function = std::function<void(NPC*)>;
    extern std::unordered_map<std::string, Function> functions;
    void addFunctions(const std::vector<std::pair<std::string, Function>>& fn_list);
}