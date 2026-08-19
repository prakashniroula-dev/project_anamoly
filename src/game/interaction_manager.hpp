#pragma once
#include <SFML/System/Vector2.hpp>
#include <string>
#include <optional>
#include <map/map_manager.hpp>

class NPC;


struct ActionHint {
    char key;               // 'E' or 'T'
    std::string label;      // e.g., "Door", "Talk", "Swap Back"
    int priority = 0;       // lower = higher priority (optional)
};

class InteractionManager {
public:
    static InteractionManager& get();

    // Call once per frame with the player's world position
    void update(const sf::Vector2f& playerPos);

    void handleKeyPress(sf::Keyboard::Key key); // handle E, T, F keys

    void interact();   // E key
    void swap(); // T key
    void inspect(); // F key

    const std::vector<ActionHint>& getHints() const { return m_hints; }

    // Getters for cached state (optional)
    NPC* getNearestNPC() const { return m_nearestNPC; }
    const std::optional<Transition>& getTransition() const { return m_nearTransition; }

private:
    InteractionManager() = default;

    void updateNearestNPC(const sf::Vector2f& playerPos);
    void updateTransition(const sf::Vector2f& playerPos);
    std::optional<std::pair<sf::Vector2f, std::string>> m_nearestInspectable;
    void updateNearestInspectable(const sf::Vector2f& playerPos);
    void buildHints();

    NPC* m_nearestNPC = nullptr;
    std::optional<Transition> m_nearTransition;
    std::vector<ActionHint> m_hints;
    bool m_canSwapBack = false;
};