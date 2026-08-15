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
    if (m_screens.empty())
        return;
    
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        bool blocks = (*it)->blocksGameUpdate();   // store flag before update
        gameShouldUpdate = !blocks;
        (*it)->update(dt);
        if ( gameShouldUpdate ) {
            break; // screens below are blocked
        }
    }
}

void UIManager::draw(sf::RenderWindow& window) const {
    auto start = m_screens.begin();
    for (auto it = m_screens.begin(); it != m_screens.end(); ++it) {
        if (!(*it)->displayBelow()) {
            start = it;
        }
    }
    for (auto it = start; it != m_screens.end(); ++it) {
        (*it)->draw(window);
    }
}