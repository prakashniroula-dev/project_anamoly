#include <ui/ui_manager.hpp>

void UIManager::pushScreen(std::unique_ptr<UIScreen> screen) {
    // if (!m_screens.empty()) {
    //     // Optionally notify current top that it's losing focus?
    // }
    m_screens.push_back(std::move(screen));
    m_screens.back()->onEnter();
}

void UIManager::popScreen() {
    if (!m_screens.empty()) {
        m_screens.back()->onExit();
        m_screens.pop_back();
    }
}

bool UIManager::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    // Iterate from top to bottom; stop if a screen consumes the event.
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        if ((*it)->handleEvent(event, window))
            return true;
        if ((*it)->blocksInput())
            break; // screens below are blocked
    }
    return false;
}

void UIManager::update(float dt, bool& gameShouldUpdate) {
    gameShouldUpdate = true;
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        (*it)->update(dt);
        if ((*it)->blocksGameUpdate()) {
            gameShouldUpdate = false;
            break;
        }
    }
}

void UIManager::draw(sf::RenderWindow& window) const {
    for (auto& screen : m_screens)
        screen->draw(window);
}